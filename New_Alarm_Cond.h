#include "errors.h"
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/** @brief Structure to store information about each alarm */
typedef struct alarm_tag {
  struct alarm_tag *link; /**< Pointer to the next alarm in the list */
  int group;          /**< Alarm group number */
  int alarm_id;       /**< Unique identifier for the alarm */
  int seconds;        /**< Seconds until the alarm goes off */
  time_t time;        /**< Time in seconds from EPOCH when the alarm is set */
  char message[128];  /**< Message associated with the alarm */
} alarm_t;

/** @brief Structure to store information about display alarm threads */
typedef struct display_alarm_info {
  pthread_t thread;           /**< Thread handling display of alarms */
  int alarm_group;            /**< Group number of alarms displayed by this thread */
  int alarms_in_group;        /**< Number of alarms in this group */
  int alarm1;                 /**< ID of the first alarm assigned to this thread */
  int alarm1_taken_over;      /**< Flag indicating if the first alarm was taken over */
  char alarm1_message[128];   /**< Message associated with the first alarm */
  int alarm2;                 /**< ID of the second alarm assigned to this thread */
  int alarm2_taken_over;      /**< Flag indicating if the second alarm was taken over */
  char alarm2_message[128];   /**< Message associated with the second alarm */
  struct display_alarm_info *next; /**< Pointer to the next thread in the list */
} display_alarm_info_t;

// List of display threads
display_alarm_info_t *display_alarm_threads;

// Mutex for display thread list, to keep track of the display thread list
pthread_mutex_t display_list_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t display_list_semaphore;    // Semaphore for display thread list
sem_t alarm_write_semaphore;     // Semaphore to write to the alarm list
sem_t alarm_read_semaphore;      // Semaphore to read from the alarm list
sem_t changed_alarm_semaphore;  // Semaphore for changed alarm list
sem_t monitor_semaphore;         // Semaphore for monitor thread to be signalled

alarm_t *alarm_list = NULL;       // Alarm List
alarm_t *changed_alarm_list = NULL;  // Alarm Change list

// Count of threads reading the alarm list
int reading_threads = 0;

/**
 * @brief Increments the count of threads reading the alarm list.
 *
 * Increments the count of display threads accessing the alarm list and locks
 * the writer semaphore if this is the first display thread reading the list.
 */
void start_reading();

/**
 * @brief Decrements the count of threads reading the alarm list.
 *
 * Decrements the count of display threads accessing the alarm list and unlocks
 * the writer semaphore if this is the last display thread reading the list.
 */
void stop_reading();

/**
 * @brief The display alarm thread function that displays alarms in its group.
 *
 * This function is responsible for displaying alarms associated with a specific
 * group number. It checks for alarms in the specified
 * group and assigned to the specific thread and displays them at 5 second increments.
 *
 */
void *display_alarm(void *args);

/**
 * @brief Monitors alarms for changes and expiration.
 *
 * This function serves as the monitor thread, continuously checking for changed
 * alarms and expired alarms. It processes changes in alarms, updates alarm
 * information, and finds the nearest alarm for expiration. The thread sleeps
 * until the next nearest alarm or until a change is signaled.
 *
 */
void *monitor_alarms(void *args);

/**
 * @brief Processes expired alarms.
 *
 * Checks for alarms that have expired based on their set time and duration.
 * Removes expired alarms from the alarm list and prints information about
 * the removed alarms. It also frees memory associated with expired alarms.
 * This function is called periodically by the monitor thread to manage
 * alarm expiration.
 *
 */
void process_expired_alarms();

/**
 * @brief Inserts a change alarm request into the changed alarm list.
 *
 * Inserts a changed alarm into the list of changed alarms to be updated and signals the
 * monitor thread to process these changes.
 *
 * @param alarm A pointer to the alarm structure with the modified data.
 */
void insert_alarm_changed(alarm_t *alarm);

/**
 * @brief Checks for or creates a display thread for the alarm group.
 *
 * Checks if a display thread exists for the alarm's group. If it does, the
 * alarm is assigned to an available thread. If not, a new thread is created for
 * the group.
 *
 * @param alarm A pointer to the alarm structure thats needs to be displayed.
 * @param taken_over An indicator whether the alarm would be taken over by
 * another thread.
 */
void check_or_create_display_thread(alarm_t *alarm, int taken_over);

/**
 * @brief Inserts a new alarm into the list of alarms.
 *
 * Inserts a new alarm into the list of alarms sorted by the alarm ID. It
 * signals the monitor thread about the new alarm and checks or creates a
 * display thread for it.
 *
 * @param alarm A pointer to the new alarm structure to be inserted.
 */
void insert_alarm(alarm_t *alarm);
