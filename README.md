This C program builds the alarm_cond.c from "Programming with POSIX Threads" by David R. Butenhof, it introduces several improvements to manage alarms efficiently using POSIX threads. It extends the original alarm system by implementing multithreading techniques to handle alarms concurrently, enhancing system responsiveness and organization. Dedicated display threads manage alarm displays, while a monitoring thread efficiently handles alarm expiration, optimizing system performance.

## Usage

1. Ensure that the header file (New_Alarm_Cond.h) is in the same directory as New_Alarm_Cond.c, compile the program using:
    `cc New_Alarm_Cond.c -D_POSIX_PTHREAD_SEMANTICS -lpthread`
2. Run the compiled executable using "a.out".
3. Follow the example commands below to manage alarms.

## Example Commands

- `Start_Alarm(1) Group (10): 10 Message`: Starts an alarm with ID 1, a display thread for group 1 will be assigned to this alarm and its message will be displayed by that thread until its removed by the monitor thread.
- `Change_Alarm(1) Group (10): 10 New_Message`: Replaces alarm 1's message with the updated message and the display thread would show that the message changed and then continue printing like normally every 5 seconds.
- `Change_Alarm(1) Group (20): 10 New_Message`: Replaces alarm with 1's group with group 20 and the alarm would be assigned to a new display thread responsible for group 20 and that has an empty alarm slot to display alarm 1's message.

## Features

2. Multithreaded Alarm Management: 
    - Utilizes POSIX threads to handle alarms concurrently, improving system responsiveness and scalability.
    - Uses dedicated display threads to manage alarm display.
    - Efficiently manages alarm expiration and changes using a monitor thread.
    - Dynamically creates multiple display threads for parallel processing.

2. Semaphore-Protected Lists:
   - Ensures thread safety with semaphore protection for access to the alarm list, changed alarm list and display thread list.

3. Efficient Sleeping Mechanism:
   - The display threads strategically sleep allowing the monitor thread sufficient time remove expiring alarms without contention.

4. Command-Driven Alarm Handling:
   - Supports two commands: `Start_Alarm`, `Change_Alarm`.
   - Provides a flexible and interactive interface for managing alarms.

5. Dynamic Alarm Insertion and Changes:
   - Intelligently handles the insertion of new alarms into the list and the changes of existing alarms.
   - Organizes alarms within the list based on their ids.

6. Alarm Expiration Mechanism:
   - Efficiently removes expiring alarms from the list.
   - Manages associated display threads and ensures proper synchronization.

7. Dynamic Display Threads Creation:
   - Display alarm threads are dynamically created based on the alarm time group.
   - Enables optimal parallel processing of alarms and adapts to varying alarms wait times.

8. Informative Display Alarm Information:
   - Periodically processes alarms, presenting detailed information such as alarm ID, thread ID, alarm time group, timestamp, and alarm message.

9. Interactive User Interface:
   - The main thread continuously awaits user input, allowing users to initiate or change  alarms without having to wait.
   - Gracefully handles invalid commands with corresponding error messages.


## Display Threads

### Dynamic Display Thread Creation

If an alarm belonging to certain group is inserted and there is no display thread responsible for that group then a new display thread is created. After being created the display thread is responsible for the following:

- Periodically checking for changes in the alarms it is responsible for.
- For every alarm in the alarm list that the display thread is responsible for it will print every 5 seconds the message for that alarm.
- Print: “Alarm (<alarm_id>) Printed by Display Thread <thread-id> > for Group(<Group_Number>) at <time>: <time message>”

### Dynamic Display Thread Termination

Display threads are terminated if there are no alarms left in the group they are responsible for.

- Print: “No More Alarms in Group(<group_id>): Display Thread <thread-id> exiting at <exit_time>”

## Monitor Thread

### Monitor Thread Responsiblity

The monitor thread is responsible for displaying checking on the existing alarms in the list and remove alarms that have expired as well as checking the changed alarms list for change requests to existing alarms and applying those changes.
