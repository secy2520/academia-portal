#ifndef PROF_FUNCTIONS
#define PROF_FUNCTIONS

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "common.h"
#include "admin.h"

#define DB_FILE "faculty_db.txt"
bool add_course(int clientSocket,struct Faculty *authenticatedprof);
bool remove_course(int clientSocket, struct Faculty *authenticatedprof);
bool authenticate_prof(int clientSocket, struct Faculty *authenticatedprof);
//bool save_prof_to_db(struct Faculty *authenticatedprof);
void view_enrollments(int clientSocket);
void view_offered(int clientSocket);
bool update_course(int clientSocket);
bool change_password(int clientSocket, struct Faculty *authenticatedprof);
bool prof_operation_handler(int clientSocket)
{
    struct Faculty authenticatedprof;

    bool authenticated = authenticate_prof(clientSocket, &authenticatedprof);

    if (authenticated)
    {
        ssize_t readBytes;
        char readbuff[1000];

        // Send a welcome message after successful login
        send(clientSocket, "Login Successfully\n", strlen("Login Successfully\n"), 0);

        while (1)
        {
            // Send the professor menu to the client
            char profMenu[] = "Professor Menu:\n1. Add new Course\n2. Remove offered course\n3. View enrollments in the course\n4. View offered course detials\n  5.Update course details\n 6. Exit\n 7.Change Password\n";
            send(clientSocket, profMenu, strlen(profMenu), 0);

            // Receive the client's menu choice
            readBytes = recv(clientSocket, readbuff, sizeof(readbuff), 0);
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for PROF_MENU");
                return false;
            }

            int choice = atoi(readbuff);

            // Handle the menu choice
            switch (choice)
            {
                case 1:
                    if (add_course(clientSocket,&authenticatedprof))
                    {
                        send(clientSocket,"Done successfully\n", strlen("Done successfully\n"), 0);
                    }
                   /* else
                     {
                     send(clientSocket,"There is some error please try again!\n", strlen("There is some error please try again\n!"), 0);
                     }*/
                    break;
               case 2:
                    if(remove_course(clientSocket,&authenticatedprof))
                     {
                     send(clientSocket,"Done successfully\n",strlen("Done successfully\n"),0);
                     }
                     else
                     {
                     send(clientSocket,"There is some error please try again!\n", strlen("There is some error please try again!\n"), 0);
                     }
                     break;
              case 3: view_enrollments(clientSocket);
                      break;
              case 4: view_offered(clientSocket);
                      break;
              case 5: if(update_course(clientSocket))
                      {
                      send(clientSocket,"Done successfully\n", strlen("Done successfully\n"), 0);
                      }
                      else
                      {
                      send(clientSocket,"There is some error please try again!\n", strlen("There is some error please try again!\n"), 0);
                      }
                     break;
              case 6:close(clientSocket);
                     break;
              case 7:
                if (change_password(clientSocket, &authenticatedprof)) {
                    send(clientSocket, "Password changed successfully\n", strlen("Password changed successfully\n"), 0);
                } else {
                    send(clientSocket, "Failed to change password\n", strlen("Failed to change password\n"), 0);
                }
                break;

              }
        }
    }
    return false;
}

bool authenticate_prof(int clientSocket, struct Faculty *authenticatedprof)
{
    char username[100];
    char pass[100];

    // Send a prompt for the username
    const char *userPrompt = "Enter username: \n";
    send(clientSocket, userPrompt, strlen(userPrompt), 0);

    // Receive the username from the client
    ssize_t bytesRead = recv(clientSocket, username, sizeof(username), 0);
    if (bytesRead <= 0)
    {
        close(clientSocket);
        return false;
    }
    if (username[bytesRead - 1] == '\n')
    {
        username[bytesRead - 1] = '\0';
    }
    else
    {
        username[bytesRead] = '\0';
    }
    // Send a prompt for the password
    const char *passPrompt = "Enter password: \n";
    send(clientSocket, passPrompt, strlen(passPrompt), 0);

    // Receive the password from the client
    bytesRead = recv(clientSocket, pass, sizeof(pass), 0);
    if (bytesRead <= 0)
    {
        close(clientSocket);
        return false;
    }
    if (pass[bytesRead - 1] == '\n')
    {
        pass[bytesRead - 1] = '\0';
    }
    else
    {
        pass[bytesRead] = '\0';
    }

    int dbFile = open(DB_FILE, O_RDWR);
    if (dbFile == -1)
    {
        perror("Error opening the database file");
        close(clientSocket);
        return false;
    }

    struct Faculty prof;
    bool found = false;
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;

    // Compare the received username and password with stored credentials
    while (read(dbFile, &prof, sizeof(struct Faculty)) > 0)
    {
        if (found == true)
            break;
        // Attempt to obtain an advisory read lock on the current record
        if (fcntl(dbFile, F_SETLK, &lock) == -1)
        {
            perror("Error obtaining advisory lock on the database record");
            close(dbFile);
            close(clientSocket);
            return false;
        }

        if (strcmp(username, prof.name) == 0 &&
            strcmp(pass, prof.password) == 0)
        {
            // Authentication successful
            *authenticatedprof = prof;
            found = true;
        }

        // Release the advisor read lock on the current record
        lock.l_type = F_UNLCK;
        if (fcntl(dbFile, F_SETLK, &lock) == -1)
        {
            perror("Error releasing advisory lock on the database record");
            close(dbFile);
            close(clientSocket);
            return false;
        }
    }

    // Close the database file
    close(dbFile);

    if (found)
    {
        // Authentication successful
        const char *successMessage = "Authentication successful\n";
        send(clientSocket, successMessage, strlen(successMessage), 0);
        return true;
    }
    else
    {
        // Authentication failed
        const char *failureMessage = "Authentication failed\n";
        send(clientSocket, failureMessage, strlen(failureMessage), 0);
        close(clientSocket);
        return false;
    }
}

bool add_course(int clientSocket,struct Faculty *authenticatedprof)
{

struct course cc;

send(clientSocket,"Enter course name:\n ", strlen("Enter course name:\n "), 0);
 //   send(clientSocket, namePrompt, strlen(namePrompt), 0);
    int readResult = recv(clientSocket, cc.name, sizeof(cc.name), 0);
    if (readResult <= 0) {
        send(clientSocket,"Error receiving faculty name from client",strlen("Error receiving faculty name from client"),0);
        return false;
    }
  cc.name[readResult]='\0';

send(clientSocket,"Enter course ID\n ", strlen("Enter course ID\n "), 0);
 //   send(clientSocket, namePrompt, strlen(namePrompt), 0);
     readResult = recv(clientSocket, cc.course_id, sizeof(cc.course_id), 0);
    if (readResult <= 0) {
        send(clientSocket,"Error receiving faculty name from client",strlen("Error receiving faculty name from client"),0);
        return false;
    }
  cc.course_id[readResult]='\0';

send(clientSocket,"Enter department offering\n ", strlen("Enter department offering\n "), 0);
 //   send(clientSocket, namePrompt, strlen(namePrompt), 0);
    readResult = recv(clientSocket, cc.department, sizeof(cc.department), 0);
    if (readResult <= 0) {
        send(clientSocket,"Error receiving dept name from client",strlen("Error receiving dept from client"),0);
        return false;
    }
cc.department[readResult]='\0';


send(clientSocket, "Enter max strength:\n ", strlen("Enter max strength:\n "), 0);
int max_strength;
readResult = recv(clientSocket, &max_strength, sizeof(max_strength), 0);

if (readResult <= 0) {
    send(clientSocket, "Error receiving max strength from client\n", strlen("Error receiving max strength from client\n"), 0);
    return false;
}

cc.max_strength = max_strength;


    const char *dbFileName = "course_db.txt";
    int dbFile = open(dbFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (dbFile == -1) {
        perror("Error opening or creating the database file");
        return false;
    }

    cc.curr_strength=0;
    struct course course_record;
    bool course_found = false;

    // Search for the course in the database
  while (read(dbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(cc.name, course_record.name) == 0)
        {
            course_found = true;
              send(clientSocket, "Sorry the course already exists!!\n", strlen("Sorry the course already exists!!\n"), 0);
              close(dbFile);
               return false;
            break;
        }
    }


    // Calculate the offset for the specific record based on its size
    off_t recordOffset = lseek(dbFile, 0, SEEK_END);

    // Lock the specific record for writing (advisory lock)
    if (flock(dbFile, LOCK_EX) == -1) {
        perror("Error locking the record for writing");
        close(dbFile);
        return false;
    }

    // Write the faculty information to the file at the calculated offset
    ssize_t writeResult = pwrite(dbFile, &cc, sizeof(struct course), recordOffset);
    if (writeResult == -1 || writeResult < sizeof(struct Faculty)) {
        perror("Error writing faculty record to database file");
        flock(dbFile, LOCK_UN); // Unlock the record before closing
        close(dbFile);
        return false;
    }

    // Unlock the specific record
    if (flock(dbFile, LOCK_UN) == -1) {
        perror("Error unlocking the record");
        close(dbFile);
        return false;
    }

    // Close the database file
    close(dbFile);

   if (authenticatedprof->num_offered_courses < MAX_OFFERED_COURSES)
    {
        strcpy(authenticatedprof->offered_courses[authenticatedprof->num_offered_courses], cc.name);
        authenticatedprof->num_offered_courses++;

        // Save the updated student information to the student database file
        if (!save_prof_to_db(authenticatedprof))
        {
            send(clientSocket, "Error saving faculty information\n", strlen("Error saving faculty information\n"), 0);
            return false;
        }

        send(clientSocket, "Course added successfully to the faculty profile\n", strlen("Course added successfully to the faculty profile\n"), 0);
        return true;
    }
    else
    {
        send(clientSocket, "You have reached the maximum number of offered courses\n", strlen("You have reached the maximum number of enrolled courses\n"),0);
        return false;
    }

return true;
}

/*bool save_prof_to_db(struct Faculty * authenticatedprof)
{ 
    int dbaFile = open(DB_FILE, O_RDWR | O_CREAT, 0666);
    if (dbaFile == -1)
    {
        perror("Error opening the database file");
        return false;
    }

    struct Faculty tempprof;
    bool recordFound = false;
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK; // Exclusive write lock
    lock.l_whence = SEEK_SET;

    // Read and update student records in the database file
    while (read(dbaFile, &tempprof, sizeof(struct Faculty)) > 0)
    {
        if (strcmp(tempprof.name, authenticatedprof->name) == 0)
        {
            // Attempt to obtain an advisory lock on the current student record
            lock.l_start = lseek(dbaFile, 0, SEEK_CUR) - sizeof(struct Faculty);
            lock.l_len = sizeof(struct Faculty);
            if (fcntl(dbaFile, F_SETLK, &lock) == -1)
            {
                perror("Error obtaining advisory lock on the database record\n");
                close(dbaFile);
                return false;
            }

            // Seek back to the beginning of the record and write the updated student data
            lseek(dbaFile, -sizeof(struct Faculty), SEEK_CUR);
            write(dbaFile, authenticatedprof, sizeof(struct Faculty));

            // Release the advisory lock on the current student record
            lock.l_type = F_UNLCK;
            if (fcntl(dbaFile, F_SETLK, &lock) == -1)
            {
                perror("Error releasing advisory lock on the database record\n");
                close(dbaFile);
                return false;
            }

            recordFound = true;
            break; // No need to continue searching
        }
    }

    // Close the database file
    close(dbaFile);

    if (!recordFound)
    {
        return false;
    }

    return true;
}*/

bool remove_course(int clientSocket, struct Faculty *authenticatedprof)
{
    char course_name[100];
    // Ask the professor for the course name to remove
    send(clientSocket, "Enter the course name to remove:\n ", strlen("Enter the course name to remove:\n "), 0);

    // Receive the course name from the professor
    ssize_t readResult = recv(clientSocket, course_name, sizeof(course_name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client\n", strlen("Error receiving course name from client\n"), 0);
        return false;
    }
    course_name[readResult] = '\0';

    // Open the course database file for reading and writing
    const char *dbFileName = "course_db.txt";
    int dbFile = open(dbFileName, O_RDWR);
    if (dbFile == -1)
    {
        perror("Error opening the database file\n");
        return false;
    }

    // Define a structure to represent a course
    struct course course_record;
    bool course_found = false;


    // Iterate through the courses in the database
    while (read(dbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(course_name, course_record.name) == 0)
        {
            // Course found; mark it as removed
            course_found = true;
            memset(course_record.name, 0, sizeof(course_record.name)); // Mark the course name as empty
            lseek(dbFile, -sizeof(struct course), SEEK_CUR);
            write(dbFile, &course_record, sizeof(struct course));
            break;
        }
    }

    // Close the course database file
    close(dbFile);

    if (course_found)
    {
        // Course removal successful
        const char *removalMessage = "Course removal successful\n";
        send(clientSocket, removalMessage, strlen(removalMessage), 0);

        // Update the professor's information in prof_db.txt
        if (authenticatedprof->num_offered_courses > 0)
        {
            // Find and remove the course from the professor's offered courses list
            for (int i = 0; i < authenticatedprof->num_offered_courses; i++)
            {
                if (strcmp(authenticatedprof->offered_courses[i], course_name) == 0)
                {
                    // Shift remaining courses to fill the gap
                    for (int j = i; j < authenticatedprof->num_offered_courses - 1; j++)
                    {
                        strcpy(authenticatedprof->offered_courses[j], authenticatedprof->offered_courses[j + 1]);
                    }
                    // Decrement the number of offered courses
                    authenticatedprof->num_offered_courses--;

                    // Save the updated professor's information to the prof_db.txt
                    if (!save_prof_to_db(authenticatedprof))
                    {
                        send(clientSocket, "Error saving professor's information\n", strlen("Error saving professor's information\n"), 0);
                        return false;
                    }
                    break;
                }
            }
        }
        return true;
    }
    else
    {
        // Course not found
        const char *courseNotFoundMessage = "Course not found\n";
        send(clientSocket, courseNotFoundMessage, strlen(courseNotFoundMessage), 0);
        return false;
    }
}

void view_enrollments(int clientSocket)
{ 
  char course_name[100];

    // Ask the professor for the course name to view enrollments
    send(clientSocket, "Enter the course name to view enrollments:\n ", strlen("Enter the course name to view enrollments:\n "), 0);

    // Receive the course name from the professor
    int readResult = recv(clientSocket, course_name, sizeof(course_name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client\n", strlen("Error receiving course name from client\n"), 0);
        
    }
    course_name[readResult] = '\0';

    // Open the course database file for reading
    const char *dbFileName = "course_db.txt";
    int dbFile = open(dbFileName, O_RDONLY);
    if (dbFile == -1)
    {
        perror("Error opening the database file");
    }

    // Define a structure to represent a course
    struct course course_record;
    bool course_found = false;

    // Iterate through the courses in the database
    while (read(dbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(course_name, course_record.name) == 0)
        {
            // Course found; display the current enrollments
            course_found = true;
            char strengthMessage[1000];
            snprintf(strengthMessage, sizeof(strengthMessage), "Current Strength for Course '%s': %d\n", course_record.name, course_record.curr_strength);

          send(clientSocket, strengthMessage, strlen(strengthMessage), 0);
            break;
        }
    }

    // Close the course database file
    close(dbFile);


    if(!course_found)
    {
        // Course not found
        const char *courseNotFoundMessage = "Course not found\n";
        send(clientSocket, courseNotFoundMessage, strlen(courseNotFoundMessage), 0);
    }

}

void view_offered(int clientSocket)
{
    char courseid[100];  // Course ID is a string, as it's more likely to be alphanumeric

    // Ask the professor for the course ID to view course details
    send(clientSocket, "Enter the course ID to view the course details\n", strlen("Enter the course ID to view the course details\n"), 0);

    // Receive the course ID from the professor
    int readResult = recv(clientSocket, courseid, sizeof(courseid), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course ID from client\n", strlen("Error receiving course ID from client\n"), 0);
        return;
    }
    courseid[readResult] = '\0';

    // Open the course database file for reading
    const char *dbFileName = "course_db.txt";
    int dbFile = open(dbFileName, O_RDONLY);
    if (dbFile == -1)
    {
        perror("Error opening the database file\n");
        send(clientSocket, "Error opening the course database\n", strlen("Error opening the course database\n"), 0);
        return;
    }

    // Define a structure to represent a course
    struct course course_record;
    bool course_found = false;

    // Iterate through the courses in the database
    while (read(dbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(courseid, course_record.course_id) == 0)
        {
            // Course found; display the course details
            course_found = true;
            char courseDetails[408];
        snprintf(courseDetails, sizeof(courseDetails), "Course ID: %s\nCourse Name: %s\nDepartment: %s\nCurrent Strength: %d\n",
        course_record.course_id, course_record.name, course_record.department, course_record.curr_strength);

     send(clientSocket, courseDetails, strlen(courseDetails), 0);
     send(clientSocket,"Max strength is: ",strlen("Max strength is: "),0);
     send(clientSocket,&course_record.max_strength,sizeof(course_record.max_strength),0);

            break;
        }
    }

    // Close the course database file
    close(dbFile);

    if (!course_found)
    {
        // Course not found
        const char *courseNotFoundMessage = "Course not found\n";
        send(clientSocket, courseNotFoundMessage, strlen(courseNotFoundMessage), 0);
    }
}

bool update_course(int clientSocket)
{
    char course_id[100];
    send(clientSocket, "Enter the course ID to update:\n ", strlen("Enter the course ID to update:\n "), 0);
    ssize_t bytesRead = recv(clientSocket, course_id, sizeof(course_id), 0);
    if (bytesRead == -1)
    {
        perror("Error receiving course ID from client\n");
        return false;
    }
    course_id[bytesRead] = '\0';

    const char *dbFileName = "course_db.txt";
    int dbFile = open(dbFileName, O_RDWR);
    if (dbFile == -1)
    {
        perror("Error opening the database file\n");
        return false;
    }

    // Lock the file for writing
    if (flock(dbFile, LOCK_EX) == -1)
    {
        perror("Error locking the file for writing\n");
        close(dbFile);
        return false;
    }

    struct course course_record;
    bool found = false;

    while (read(dbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(course_record.course_id, course_id) == 0)
        {
            found = true;

            
            send(clientSocket, "Enter updated course name:\n ", strlen("Enter updated course name:\n "), 0);
            bytesRead = recv(clientSocket, course_record.name, sizeof(course_record.name) - 1, 0);
            if (bytesRead <= 0)
            {
                send(clientSocket, "Error receiving updated course name from client\n", strlen("Error receiving updated course name from client\n"), 0);
                return false;
            }
            course_record.name[bytesRead] = '\0';

            send(clientSocket, "Enter updated department:\n ", strlen("Enter updated department:\n "), 0);
            bytesRead = recv(clientSocket, course_record.department, sizeof(course_record.department) - 1, 0);
            if (bytesRead <= 0)
            {
                send(clientSocket, "Error receiving updated department from client\n", strlen("Error receiving updated department from client\n"), 0);
                return false;
            }
            course_record.department[bytesRead] = '\0';

            send(clientSocket, "Enter updated max strength:\n ", strlen("Enter updated max strength:\n "), 0);
            bytesRead = recv(clientSocket, &course_record.max_strength, sizeof(course_record.max_strength), 0);
            if (bytesRead <= 0)
            {
                send(clientSocket, "Error receiving updated max strength from client\n", strlen("Error receiving updated max strength from client\n"), 0);
                return false;
            }

            // Move the file pointer back to the beginning of the record and update it
            lseek(dbFile, -sizeof(struct course), SEEK_CUR);
            write(dbFile, &course_record, sizeof(struct course));
            break;
        }
    }

    if (found)
    {
        // Course record updated successfully
        const char *updateMessage = "Course record updated successfully\n";
        send(clientSocket, updateMessage, strlen(updateMessage), 0);
    }
    else
    {
        // Course not found
        const char *notFoundMessage = "Course not found\n";
        send(clientSocket, notFoundMessage, strlen(notFoundMessage), 0);
    }

    // Unlock the file
    if (flock(dbFile, LOCK_UN) == -1)
    {
        perror("Error unlocking the file\n");
    }

    // Close the file
    close(dbFile);
    return found;
}



bool change_password(int clientSocket, struct Faculty *authenticatedprof) {
    char new_password[100];

    send(clientSocket, "Enter a new password: \n", strlen("Enter a new password: \n"), 0);
    ssize_t bytesRead = recv(clientSocket, new_password, sizeof(new_password), 0);

    if (bytesRead <= 0) {
        send(clientSocket, "Error receiving new password from client\n", strlen("Error receiving new password from client\n"), 0);
        return false;
    }

    if (new_password[bytesRead - 1] == '\n') {
        new_password[bytesRead - 1] = '\0';
    } else {
        new_password[bytesRead] = '\0';
    }

    // Update the professor's password
    strncpy(authenticatedprof->password, new_password, sizeof(authenticatedprof->password));

    // Save the updated professor's information to the database
    if (!save_prof_to_db(authenticatedprof)) {
        send(clientSocket, "Error saving faculty information\n", strlen("Error saving faculty information\n"), 0);
        return false;
    }

    return true;
}
#endif
