#include "New_Alarm_Cond.h"

/*
 * New_Alarm_Cond.c
 *
 * This program is an enhancement of the alarm_cond.c program
 * sourced from "Programming with POSIX Threads" by David R. Butenhof. It greatly
 * augments the original alarm system by introducing multithreading via POSIX
 * threads, creating a more organized and readable structure. This improved
 * version manages alarms concurrently by employing specialized threads for alarm
 * display, synchronizing critical sections using semaphores to prevent
 * conflicts in accessing shared resources, and utilizing a monitoring thread to
 * efficiently handle alarm expiration. Through command-line inputs, users can
 * add, modify, and display alarms. Overall, this code establishes a robust and
 * synchronized alarm system capable of managing multiple alarms concurrently,
 * ensuring proper synchronization and error handling for reliable execution.
 */

void start_reading() {
  // Increment the count of display threads
  sem_wait(&alarm_read_semaphore);
  reading_threads++;
  // If this is the first display thread, lock the alarm (writer) semaphore
  if (reading_threads == 1) {
    sem_wait(&alarm_write_semaphore);
  }
  sem_post(&alarm_read_semaphore);
}

void stop_reading() {
  sem_wait(&alarm_read_semaphore);
  reading_threads--;
  // Last display thread
  if (reading_threads == 0) {
    sem_post(&alarm_write_semaphore);
  }
  sem_post(&alarm_read_semaphore);
}

void *display_alarm(void *args) {
  // Extract the thread ID and alarm group for this display thread
  pthread_t self_id = pthread_self();

  // Local variables to store information
  int local_alarm1 = -1, local_alarm2 = -1;

  int local_alarm_group = -1;
  int local_alarm1_taken_over = 0, local_alarm2_taken_over = 0;
  char local_alarm1_message[128];
  char local_alarm2_message[128];

  while (1) {
    // Sleep for 5 seconds first
    sleep(5);

    // Lock the display list semaphore to access the display alarm list
    sem_wait(&display_list_semaphore);

    // Search for this thread's entry in the display alarm list
    display_alarm_info_t *current_thread = display_alarm_threads;
    display_alarm_info_t *prev_thread = NULL;

    while (current_thread != NULL) {
      if (pthread_equal(current_thread->thread, self_id)) {
        // Found the current thread's entry in the list, update local variables
        local_alarm_group = current_thread->alarm_group;
        local_alarm1 = current_thread->alarm1;
        local_alarm2 = current_thread->alarm2;
        local_alarm1_taken_over = current_thread->alarm1_taken_over;
        local_alarm2_taken_over = current_thread->alarm2_taken_over;
        strcpy(local_alarm1_message, current_thread->alarm1_message);
        strcpy(local_alarm2_message, current_thread->alarm2_message);
        break;
      }
      prev_thread = current_thread;
      current_thread = current_thread->next;
    }

    int old_local_alarm1 = local_alarm1;
    int old_local_alarm2 = local_alarm2;

    // Lock the semaphore to access the alarm list
    start_reading();

    // Check if local_alarm1 is not -1
    if (local_alarm1 != -1) {
      alarm_t *current_alarm = alarm_list;
      int alarm1_found = 0;
      while (current_alarm != NULL) {
        if (current_alarm->alarm_id == local_alarm1) {
          if (current_alarm->group == local_alarm_group) {
            if (local_alarm1_taken_over) {
              // Taken over alarm
              // Print the alarm message when taken oven
              printf("Display Thread %lu Has Taken Over Printing Message of "
                     "Alarm(%d) at %ld: Changed Group(%d) %d %s\n",
                     self_id, current_alarm->alarm_id, time(NULL),
                     local_alarm_group, current_alarm->seconds,
                     current_alarm->message);
              alarm1_found = 1;
            } else {
              if (strcmp(local_alarm1_message, current_alarm->message) != 0) {
                // Message changed
                printf("Display Thread %lu Starts to Print Changed Message of "
                       "Alarm(%d) at %ld: Group(%d) %d %s\n",
                       self_id, current_alarm->alarm_id, time(NULL),
                       local_alarm_group, current_alarm->seconds,
                       current_alarm->message);
                alarm1_found = 1;
                // Copy current_alarm->message to local_alarm1_message
                strcpy(local_alarm1_message, current_alarm->message);

              } else {
                // Print the alarm message not taken oven, same message
                printf("Alarm (%d) Printed by Alarm Display Thread %lu at %ld: "
                       "Group(%d) %d %s\n",
                       local_alarm1, self_id, time(NULL), local_alarm_group,
                       current_alarm->seconds, current_alarm->message);
              }
            }
            alarm1_found = 1;
            break;
          } else {
            // Alarm found but group has changed,
            printf("Display Thread %lu Has Stopped Printing Message of "
                   "Alarm(%d) at %ld: Changed Group(%d) %s\n",
                   self_id, local_alarm1, time(NULL), local_alarm_group,
                   local_alarm1_message);
            // Update local variables and thread information
            current_thread->alarms_in_group--;
            alarm1_found = 1;
            local_alarm1 = -1;
            local_alarm1_taken_over = 0;
            break;
          }
        }
        current_alarm = current_alarm->link;
      }
      // Alarm removed from list
      if (!alarm1_found) {
        // alarm1 is no longer in the alarm
        printf("Display Thread %lu Has Stopped Printing Message of Alarm(%d) "
               "at %ld: Group(%d) %s\n",
               self_id, local_alarm1, time(NULL), local_alarm_group,
               local_alarm1_message);
        // Update local variables and thread information
        current_thread->alarms_in_group--;
        local_alarm1 = -1;
        local_alarm1_taken_over = -1;
      }
    }

    // Similarly, perform the same steps for local_alarm2
    // Check if local_alarm2 is not -1
    if (local_alarm2 != -1) {
      alarm_t *current_alarm = alarm_list;
      int alarm2_found = 0;
      while (current_alarm != NULL) {
        if (current_alarm->alarm_id == local_alarm2) {
          if (current_alarm->group == local_alarm_group) {
            if (local_alarm2_taken_over) {
              // Taken over alarm
              // Print the alarm message when taken over
              printf("Display Thread %lu Has Taken Over Printing Message of "
                     "Alarm(%d) at %ld: Changed Group(%d) %d %s\n",
                     self_id, current_alarm->alarm_id, time(NULL),
                     local_alarm_group, current_alarm->seconds,
                     current_alarm->message);
              alarm2_found = 1;
            } else {
              if (strcmp(local_alarm2_message, current_alarm->message) != 0) {
                // Message changed
                printf("Display Thread %lu Starts to Print Changed Message of "
                       "Alarm(%d) at %ld: Group(%d) %d %s\n",
                       self_id, current_alarm->alarm_id, time(NULL),
                       local_alarm_group, current_alarm->seconds,
                       current_alarm->message);
                alarm2_found = 1;
                // Copy current_alarm->message to local_alarm2_message
                strcpy(local_alarm2_message, current_alarm->message);
              } else {
                // Print the alarm message not taken over, same message
                printf("Alarm (%d) Printed by Alarm Display Thread %lu at %ld: "
                       "Group(%d) %d %s\n",
                       local_alarm2, self_id, time(NULL), local_alarm_group,
                       current_alarm->seconds, current_alarm->message);
              }
            }
            alarm2_found = 1;
            break;
          } else {
            // Alarm found but group has changed,
            printf("Display Thread %lu Has Stopped Printing Message of "
                   "Alarm(%d) at %ld: Changed Group(%d) %s\n",
                   self_id, local_alarm2, time(NULL), local_alarm_group,
                   local_alarm2_message);
            // Update local variables and thread information
            current_thread->alarms_in_group--;
            alarm2_found = 1;
            local_alarm2 = -1;
            local_alarm2_taken_over = 0;
            break;
          }
        }
        current_alarm = current_alarm->link;
      }
      // Alarm removed from list
      if (!alarm2_found) {
        // alarm2 is no longer in the alarm
        printf("Display Thread %lu Has Stopped Printing Message of Alarm(%d) "
               "at %ld: Group(%d) %s\n",
               self_id, local_alarm2, time(NULL), local_alarm_group,
               local_alarm2_message);
        // Update local variables and thread information
        current_thread->alarms_in_group--;
        local_alarm2 = -1;
        local_alarm2_taken_over = -1;
      }
    }
    // Unlock the semaphore to release the alarm list
    stop_reading();

    // Check if both alarms were reassigned
    if (local_alarm1 == -1 && local_alarm2 == -1) {
      printf(
          "No More Alarms in Group(%d): Display Thread %ld exiting at %lu.\n",
          local_alarm_group, self_id, time(NULL));
      // Update pointers to remove the node
      if (prev_thread != NULL) {
        prev_thread->next = current_thread->next;
      } else {
        // If the current thread is the head of the list
        display_alarm_threads = current_thread->next;
      }
      // Free memory from the current thread
      free(current_thread);
      // Release the display list semaphore after accessing the list
      sem_post(&display_list_semaphore);
      pthread_exit(NULL); // Terminate the thread
    }

    // Update the content of the thread
    current_thread->alarm1 = local_alarm1;
    current_thread->alarm2 = local_alarm2;
    current_thread->alarm1_taken_over = local_alarm1_taken_over;
    current_thread->alarm2_taken_over = local_alarm2_taken_over;
    strcpy(current_thread->alarm1_message, local_alarm1_message);
    strcpy(current_thread->alarm2_message, local_alarm2_message);
    // Release the display list semaphore after accessing the list
    sem_post(&display_list_semaphore);
  }

  return NULL;
}

void *monitor_alarms(void *args) {
  // Wait for a new alarm insertion in the alarm list of changed alarm list
  sem_wait(&monitor_semaphore);

  while (1) {
    // Check for changed alarm requests and process them if any
    sem_wait(&changed_alarm_semaphore);

    while (changed_alarm_list != NULL) {
      alarm_t *current = changed_alarm_list;
      alarm_t *prev = NULL;
      //Lock the semaphore to write to the alarm list
      sem_wait(&alarm_write_semaphore);
      
      while (current != NULL) {
        // Find the corresponding alarm in the alarm list
        alarm_t *alarm_to_change = NULL;
        alarm_t *alarm_iterator = alarm_list;

        while (alarm_iterator != NULL) {
          if (current->alarm_id == alarm_iterator->alarm_id) {
            alarm_to_change = alarm_iterator;
            break;
          }
          alarm_iterator = alarm_iterator->link;
        }

        if (alarm_to_change != NULL) {
          // Replace values in the corresponding alarm with Change_Alarm request
          // values
          int old_group = alarm_to_change->group;
          alarm_to_change->group = current->group;
          alarm_to_change->seconds = current->seconds;
          alarm_to_change->time = time(NULL);
          strcpy(alarm_to_change->message, current->message);

          // Print change message
          printf("Alarm Monitor Thread %ld Has Changed Alarm(%d) at %ld: "
                 "Group(%d) "
                 "%d %s\n",
                 pthread_self(), alarm_to_change->alarm_id, time(NULL),
                 alarm_to_change->group, alarm_to_change->seconds,
                 alarm_to_change->message);
          if(old_group != alarm_to_change->group){
            check_or_create_display_thread(alarm_to_change, 1);
          }
        } else {
          // Print invalid change alarm message
          printf("Invalid Change Alarm Request(%d) at %ld: Group(%d) %d %s\n",
                 current->alarm_id, time(NULL), current->group,
                 current->seconds, current->message);
        }

        // Remove the Change_Alarm request from the list
        if (prev == NULL) {
          changed_alarm_list = current->link;
          free(current);
          current = changed_alarm_list;
        } else {
          prev->link = current->link;
          free(current);
          current = prev->link;
        }
      }
      //Unlock the semaphore to write to the alarm list
      sem_post(&alarm_write_semaphore);
    }

    sem_post(&changed_alarm_semaphore);

    // Variables to find the closest alarm for display
    alarm_t *closest_alarm = NULL;
    time_t closest_expiration_time = 0;

    // Lock the semaphore to prevent changes while checking for the nearest
    // alarm
    start_reading();
    // Check if there are alarms in the list
    if (alarm_list != NULL) {
      // Iterate through the alarm list to find the closest alarm
      alarm_t *curr_alarm = alarm_list;
      closest_alarm = curr_alarm;
      closest_expiration_time = curr_alarm->time + curr_alarm->seconds;

      while (curr_alarm != NULL) {
        time_t expiration_time = curr_alarm->time + curr_alarm->seconds;
        if (expiration_time < closest_expiration_time) {
          closest_alarm = curr_alarm;
          closest_expiration_time = expiration_time;
        }
        curr_alarm = curr_alarm->link;
      }
    }
    // Release the semaphore after finding the nearest alarm
    stop_reading();
    int sleep_seconds = 0;

    if (closest_alarm != NULL) {
      // Calculate the time to sleep until the closest alarm
      sleep_seconds = closest_expiration_time - time(NULL);
      if (sleep_seconds < 0) {
        // If the alarm is already expired, set sleep time to 0
        sleep_seconds = 0;
      }
    }

    if (sleep_seconds > 0) {
      struct timespec sleep_time = {sleep_seconds, 0};

      // Wait for either the nearest alarm or a signal indicating a change
      int result = sem_timedwait(&monitor_semaphore, &sleep_time);

      if (result == -1 && errno != ETIMEDOUT) {
        perror("Semaphore wait error");
      }
    }
    //Check if there are alarms in the list that could have expired
    if(alarm_list != NULL){
      process_expired_alarms();
    }else{
      // If there are no alarms in the list, wait on the monitor semaphore
      sem_wait(&monitor_semaphore);
    }
  }
}

void process_expired_alarms() {
  time_t current_time = time(NULL);

  // Lock the semaphore to access the alarm list
  sem_wait(&alarm_write_semaphore);
  alarm_t *prev = NULL;
  alarm_t *current = alarm_list;

  while (current != NULL) {
    time_t expiration_time = current->time + current->seconds;

    if (expiration_time <= current_time) {
      // Remove the expired alarm from the list
      if (prev == NULL) {
        alarm_list = current->link;
      } else {
        prev->link = current->link;
      }

      printf("Alarm Monitor Thread %ld Has Removed Alarm(%d) at %ld: "
             "Group(%d) %d %s\n",
             pthread_self(), current->alarm_id, current_time, current->group,
             current->seconds, current->message);

      free(current);
      current = (prev == NULL) ? alarm_list : prev->link;
    } else {
      prev = current;
      current = current->link;
    }
  }

  // Release the semaphore after processing expired alarms
  sem_post(&alarm_write_semaphore);
}

void insert_alarm_changed(alarm_t *alarm) {
  int status;
  alarm_t **last, *next;

  // Lock the changed alarms list mutex
  status = sem_wait(&changed_alarm_semaphore);
  if (status != 0) {
    err_abort(status, "Lock mutex");
  }

  // Initialize pointers for traversal in the changed alarms list
  last = &changed_alarm_list;
  next = *last;

  while (next != NULL) {
    if (alarm->alarm_id < next->alarm_id) {
      // Insert before next in the changed alarms list
      alarm->link = next;
      *last = alarm;
      break;
    } else {
      // Move to the next item in the changed alarms list
      last = &next->link;
      next = *last;
    }
  }

  // If we reach the end of the list, insert the alarm into changed alarms
  if (next == NULL) {
    *last = alarm;
    alarm->link = NULL;
  }
  alarm->time = time(NULL);
  printf("Change Alarm Request(%d) Inserted by Main Thread %ld into Alarm List "
         "at %ld: Group(%d) %d %s\n",
         alarm->alarm_id, pthread_self(), time(NULL), alarm->group,
         alarm->seconds, alarm->message);

  // Unlock the changed alarms list mutex
  sem_post(&changed_alarm_semaphore);
  sem_post(&monitor_semaphore);
}

void check_or_create_display_thread(alarm_t *alarm, int taken_over) {
  int status;
  display_alarm_info_t *new_display_thread = NULL;

  sem_wait(&display_list_semaphore); // Lock to ensure thread safety

  // Check if a display thread for the group already exists
  display_alarm_info_t *current = display_alarm_threads;
  while (current != NULL) {
    if (current->alarm_group == alarm->group && current->alarms_in_group < 2) {
      // Found a display thread assigned to the group with an empty slot
      if (current->alarm1 == -1) {
        // Slot 1 is empty, assign the alarm_id to alarm1
        current->alarm1 = alarm->alarm_id;
        current->alarm1_taken_over = taken_over;
        strcpy(current->alarm1_message, alarm->message);
      } else {
        // Slot 2 is empty, assign the alarm_id to alarm2
        current->alarm2 = alarm->alarm_id;
        current->alarm2_taken_over = taken_over;
        strcpy(current->alarm2_message, alarm->message);
      }
      current->alarms_in_group++; // Increment the count of alarms in the group
      // Print the creation message
      printf("Main Thread %lu Assigned to Display Alarm Thread %lu at %ld: "
             "Group(%d) %d %s\n",
             pthread_self(), current->thread, time(NULL), alarm->group,
             alarm->seconds, alarm->message);
      sem_post(&display_list_semaphore);
      return;
    }
    current = current->next; // Move to the next display thread
  }

  // No available display thread for the group, create a new one
  new_display_thread = malloc(sizeof(display_alarm_info_t));
  if (new_display_thread == NULL) {
    // Handle memory allocation failure
    sem_post(&display_list_semaphore); // Unlock before returning
    return;
  }

  // Initialize the new display thread
  new_display_thread->alarm_group = alarm->group;
  new_display_thread->alarms_in_group = 1;
  new_display_thread->alarm1 = alarm->alarm_id;
  new_display_thread->alarm1_taken_over = taken_over;
  strcpy(new_display_thread->alarm1_message, alarm->message);

  new_display_thread->alarm2 = -1; // Initialize to -1 indicating an empty slot
  new_display_thread->alarm2_taken_over = -1;

  // Add the new display thread at the beginning of the list
  new_display_thread->next = display_alarm_threads;
  display_alarm_threads = new_display_thread;

  /* Create new thread */
  status =
      pthread_create(&new_display_thread->thread, NULL, display_alarm, NULL);
  if (status != 0) {
    // Handle thread creation failure
    free(new_display_thread);          // Free memory in case of failure
    sem_post(&display_list_semaphore); // Unlock before returning
    return;
  }

  // Print the creation message
  printf("Main Thread Created New Display Alarm Thread %lu For Alarm(%d) at "
         "%ld: Group(%d) %d %s\n",
         new_display_thread->thread, alarm->alarm_id, time(NULL), alarm->group,
         alarm->seconds, alarm->message);
  sem_post(&display_list_semaphore); // Unlock before returning
}

void insert_alarm(alarm_t *alarm) {
  int status;
  alarm_t **last, *next;

  // lock the alarm list mutex
  status = sem_wait(&alarm_write_semaphore);
  if (status != 0) {
    err_abort(status, "Lock semap");
  }

  // last is the header of list, next is the first element in list
  last = &alarm_list;
  next = *last;

  while (next != NULL) {
    if (alarm->alarm_id == next->alarm_id) {
      // An alarm with the same ID already exists, don't insert the new alarm
      printf("An alarm with ID %d already exists.\n", alarm->alarm_id);
      free(alarm); // Free the new alarm
      sem_post(&alarm_write_semaphore);
      return; // Return without inserting the new alarm
    } else if (alarm->alarm_id < next->alarm_id) {
      // Insert before next, update header
      alarm->link = next;
      *last = alarm;
      break;
    } else {
      // Move to the next item
      last = &next->link;
      next = *last;
    }
  }

  /*
   * If we reach the end of the list, insert the alarm there, next == NULL,
   * and last points to the last item or the list header.
   */
  if (next == NULL) {
    *last = alarm;
    alarm->link = NULL;
  }
  alarm->time = time(NULL);
  printf("Alarm(%d) Inserted by Main Thread %ld Into Alarm List at %ld: "
         "Group(%d) %d %s\n",
         alarm->alarm_id, pthread_self(), time(NULL), alarm->group,
         alarm->seconds, alarm->message);

  // Unlock the alarm list semaphore
  sem_post(&alarm_write_semaphore);

  // Check if a display thread needs to be created or an existing one can be
  // used
  check_or_create_display_thread(alarm, 0);
  sem_post(&monitor_semaphore); // signal monitor
}

int main(int argc, char *argv[]) {
  int status;
  char line[128];
  alarm_t *alarm;
  pthread_t monitor_thread;

  // Initialize to 1 for mutual exclusion
  sem_init(&display_list_semaphore, 0, 1);
  sem_init(&alarm_write_semaphore, 0, 1);
  sem_init(&alarm_read_semaphore, 0, 1);
  sem_init(&changed_alarm_semaphore, 0, 1);

  // Initialize to 0 for waiting
  sem_init(&monitor_semaphore, 0, 0);

  pthread_create(&monitor_thread, NULL, monitor_alarms, NULL);

  while (1) {
    printf("Alarm>\n");
    if (fgets(line, sizeof(line), stdin) == NULL)
      exit(0);
    if (strlen(line) <= 1) {
      continue;
    }
    // Allocate new alarm
    alarm = (alarm_t *)malloc(sizeof(alarm_t));
    if (alarm == NULL)
      errno_abort("Allocate alarm");

    /*
     * Parse input line into alarm_id (%d), seconds (%d), and a message
     * (%128[^\n]), consisting of up to 128 characters separated by
     * whitespace.
     */
    // COMMAND 1: Start_Alarm
    if (sscanf(line, "Start_Alarm(%d): Group(%d) %d %128[^\n]",
               &alarm->alarm_id, &alarm->group, &alarm->seconds,
               alarm->message) == 4) {
      if (alarm->alarm_id >= 0 && alarm->seconds > 0 && alarm->group >= 0) {
        // Valid alarm_id, seconds, and group, proceed with adding the alarm

        // Insert the new alarm into the list of alarms, sorted by alarm id
        insert_alarm(alarm);
      } else {
        // Invalid alarm_id or seconds
        if (alarm->alarm_id < 0) {
          fprintf(stderr, "Alarm ID must be greater than or equal to 0 0\n");
        }
        if (alarm->seconds <= 0) {
          fprintf(stderr, "Alarm time must be greater than 0\n");
        }
        if (alarm->group < 0) {
          fprintf(stderr, "Group ID must be greater than or equal to 0\n");
        }
        // Free the invalid alarm
        free(alarm);
      }
    }
    // COMMAND 2: Change Alarm
    else if (sscanf(line, "Change_Alarm(%d): Group(%d) %d %128[^\n]",
                    &alarm->alarm_id, &alarm->group, &alarm->seconds,
                    alarm->message) == 4) {
      if (alarm->alarm_id >= 0 && alarm->seconds > 0 && alarm->group >= 0) {
        // Valid alarm_id and seconds, proceed with replacing the alarm
        insert_alarm_changed(alarm);
      } else {
        // Invalid alarm_id or seconds
        if (alarm->alarm_id < 0) {
          fprintf(stderr, "Alarm ID must be greater than or equal to 0\n");
        }
        if (alarm->seconds <= 0) {
          fprintf(stderr, "Alarm time must be greater than 0\n");
        }
        if (alarm->group < 0) {
          fprintf(stderr, "Group ID must be greater than or equal to 0\n");
        }
      }
    } else {
      fprintf(stderr, "Bad command\n");
      free(alarm); // Free the invalid alarm
    }
    fflush(stdout); // Manually flush the stdout buffer
  }
}