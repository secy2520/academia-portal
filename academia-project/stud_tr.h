#ifndef STUD_FUNCTIONS
#define STUD_FUNCTIONS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "common.h"

//#define MAX_ENROLLED_COURSES 6
#define DATABASE_FILE "student_db.txt"


bool authenticate_stud(int clientSocket, struct student *authenticatedStudent);
bool enrollments(int clientSocket, struct student *authenticatedStudent);
//bool save_student_to_db(struct student *student);
bool unenroll(int clientSocket, struct student * authenticatedStudent);
void view_enrolled(int clientSocket, struct student *authenticatedstudent);
bool update_password(int clientSocket, struct student *authenticatedstudent);
bool stud_operation_handler(int clientSocket)
{
    struct student authenticatedStudent;

    bool authenticated = authenticate_stud(clientSocket, &authenticatedStudent);
    if (authenticated)
    {
        ssize_t writeBytes, readBytes;
        char readbuff[1000], writebuff[1000];

        while (1)
        {
            char studentMenu[] = "Do you want to:\n - 1.Enroll in new courses\n - 2.Unenroll from already enrolled courses\n - 3.View enrolled courses\n 4.Exit\n 5.Change Password\n";
            send(clientSocket, studentMenu, strlen(studentMenu), 0);
            readBytes = recv(clientSocket, readbuff, sizeof(readbuff), 0);
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for student menu");
                return false;
            }

            int choice = atoi(readbuff);
            switch (choice)
            {
            case 1:
                if (enrollments(clientSocket, &authenticatedStudent))
                {
                    send(clientSocket, "Done successfully\n", strlen("Done successfully\n"), 0);
                }
                else
                {
                send(clientSocket,"There is some error please try again!\n", strlen("There is some error please try again\n!"), 0);
                }
                break;
           case 2:
              if (unenroll(clientSocket, &authenticatedStudent))
              {
               send(clientSocket, "Done successfully\n", strlen("Done successfully\n"), 0);
              }
             else
              {
              send(clientSocket,"There is some error please try again!\n", strlen("There is some error please try again\n!"), 0);
              }
              break;
            case 3: view_enrolled(clientSocket, &authenticatedStudent);
                     break;
            case 4:close(clientSocket);
                 break;
            case 5:
    if (update_password(clientSocket, &authenticatedStudent)) {
        send(clientSocket, "Password updated successfully\n", strlen("Password updated successfully\n"), 0);
    } else {
        send(clientSocket, "Error updating password\n", strlen("Error updating password\n"), 0);
    }
    break;
            }
        }
    }
    return false;
}

bool authenticate_stud(int clientSocket, struct student *authenticatedStudent)
{
    char username[100];
    char pass[100];

    // Send a prompt for the username
    const char *userPrompt = "Enter username: ";
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
    const char *passPrompt = "Enter password: ";
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

    int dbFile = open(DATABASE_FILE, O_RDWR);
    if (dbFile == -1)
    {
        perror("Error opening the database file");
        close(clientSocket);
        return false;
    }

    struct student stud;
    bool found = false;
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;

    // Compare the received username and password with stored credentials
    while (read(dbFile, &stud, sizeof(struct student)) > 0)
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

        if (strcmp(username, stud.name) == 0 &&
            strcmp(pass, stud.password) == 0)
        {
            // Authentication successful
            *authenticatedStudent = stud;
            found = true;
        }

        // Release the advisory read lock on the current record
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
        const char *successMessage = "Authentication successful";
        send(clientSocket, successMessage, strlen(successMessage), 0);
        return true;
    }
    else
    {
        // Authentication failed
        const char *failureMessage = "Authentication failed";
        send(clientSocket, failureMessage, strlen(failureMessage), 0);
        close(clientSocket);
        return false;
    }
}


bool enrollments(int clientSocket, struct student *authenticatedStudent)
{
    struct course newCourse;

    // Receive the course details from the client
    send(clientSocket, "Enter course name: ", strlen("Enter course name: "), 0);
    ssize_t readResult = recv(clientSocket, newCourse.name, sizeof(newCourse.name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client", strlen("Error receiving course name from client"), 0);
        return false;
    }
    newCourse.name[readResult] = '\0';

    // Check if the student is already enrolled in this course
    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            send(clientSocket, "You are already enrolled in this course", strlen("You are already enrolled in this course"), 0);
            return false;
        }
    }

    // Open the course database file for reading and writing
    const char *courseDbFileName = "course_db.txt";
    int courseDbFile = open(courseDbFileName, O_RDWR);
    if (courseDbFile == -1)
    {
        perror("Error opening the course database file");
        return false;
    }

    struct course course_record;
    bool course_found = false;

    // Search for the course in the database
    while (read(courseDbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(newCourse.name, course_record.name) == 0)
        {
             
            
            if ((course_record.curr_strength - course_record.max_strength) >= 0)
            {
                send(clientSocket, "This course is full. You cannot enroll.", strlen("This course is full. You cannot enroll"), 0);
                close(courseDbFile);
                return false;
            }

            // Add the course to the student's list of enrolled courses
            if (authenticatedStudent->num_enrolled_courses < MAX_ENROLLED_COURSES)
            {
                strcpy(authenticatedStudent->enrolled_courses[authenticatedStudent->num_enrolled_courses], newCourse.name);
                authenticatedStudent->num_enrolled_courses++;

                // Save the updated student information to the student database file
                if (!save_student_to_db(authenticatedStudent))
                {
                    send(clientSocket, "Error saving student information", strlen("Error saving student information"), 0);
                    close(courseDbFile);
                    return false;
                }
                // Increment the current strength of the course
                course_record.curr_strength++;
                lseek(courseDbFile, -sizeof(struct course), SEEK_CUR);
                write(courseDbFile, &course_record, sizeof(struct course));

                send(clientSocket, "Course enrolled successfully", strlen("Course enrolled successfully"), 0);
            }
            else
            {
                send(clientSocket, "You have reached the maximum number of enrolled courses", strlen("You have reached the maximum number of enrolled courses"), 0);
            }
            close(courseDbFile);
            return true;
        }
    }
    close(courseDbFile);

    if (!course_found)
    {
        send(clientSocket, "Course not found", strlen("Course not found"), 0);
        return false;
    }
}



/*bool enrollments(int clientSocket, struct student *authenticatedStudent)
{
    struct course newCourse;

    // Receive the course details from the client
    send(clientSocket, "Enter course name: ", strlen("Enter course name: "), 0);
    ssize_t readResult = recv(clientSocket, newCourse.name, sizeof(newCourse.name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client", strlen("Error receiving course name from client"), 0);
        return false;
        
    }
    newCourse.name[readResult] = '\0';
   struct course course_record;

 const char *courseDbFileName = "course_db.txt";
    int courseDbFile = open(courseDbFileName, O_RDWR);
    if (courseDbFile == -1)
    {
        perror("Error opening the course database file");
        return false;
    }

if (course_record.curr_strength >= course_record.max_strength)
{
    send(clientSocket, "This course is full. You cannot enroll.", strlen("This course is full. You cannot enroll"), 0);
    return false;
}


    // Check if the student is already enrolled in this course
    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            send(clientSocket, "You are already enrolled in this course", strlen("You are already enrolled in this course"), 0);
            return false;
        }
    }

    // Open the course database file for reading and writing
   

 
   
    bool course_found = false;

    while (read(courseDbFile, &course_record, sizeof(struct course)) > 0)
    {
        if (strcmp(newCourse.name, course_record.name) == 0)
        {
            course_found = true;

            if (course_record.curr_strength >= course_record.max_strength)
            {
                send(clientSocket, "This course is full. You cannot enroll.", strlen("This course is full. You cannot enroll."), 0);
                close(courseDbFile);
                return false;
                break;
            }

            
            course_record.curr_strength++;

           
            lseek(courseDbFile, -sizeof(struct course), SEEK_CUR);
            write(courseDbFile, &course_record, sizeof(struct course));

            break;
        }
    }

    // Close the course database file
    close(courseDbFile);

    if (!course_found)
    {
        send(clientSocket, "Course not found", strlen("Course not found"), 0);
        return false;
    }

    // Add the course to the student's list of enrolled courses
    if (authenticatedStudent->num_enrolled_courses < MAX_ENROLLED_COURSES)
    {
        strcpy(authenticatedStudent->enrolled_courses[authenticatedStudent->num_enrolled_courses], newCourse.name);
        authenticatedStudent->num_enrolled_courses++;

        // Save the updated student information to the student database file
        if (!save_student_to_db(authenticatedStudent))
        {
            send(clientSocket, "Error saving student information", strlen("Error saving student information"), 0);
            return false;
        }

        send(clientSocket, "Course enrolled successfully", strlen("Course enrolled successfully"), 0);
        return true;
    }
    else
    {
        send(clientSocket, "You have reached the maximum number of enrolled courses", strlen("You have reached the maximum number of enrolled courses"), 0);
        return false;
    }

}*/

/*bool save_student_to_db(struct student *student)
{
    int dbFile = open(DATABASE_FILE, O_RDWR | O_CREAT,0666);
    if (dbFile == -1)
    {
        perror("Error opening the database file");
        return false;
    }

    struct student tempStudent;
    bool recordFound = false;
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK; // Exclusive write lock
    lock.l_whence = SEEK_SET;

    // Read and update student records in the database file
    while (read(dbFile, &tempStudent, sizeof(struct student)) > 0)
    {
        if (strcmp(tempStudent.name, student->name) == 0)
        {
            // Attempt to obtain an advisory lock on the current student record
            lock.l_start = lseek(dbFile, 0, SEEK_CUR) - sizeof(struct student);
            lock.l_len = sizeof(struct student);
            if (fcntl(dbFile, F_SETLK, &lock) == -1)
            {
                perror("Error obtaining advisory lock on the database record");
                close(dbFile);
                return false;
            }

            // Seek back to the beginning of the record and write the updated student data
            lseek(dbFile, -sizeof(struct student), SEEK_CUR);
            write(dbFile, student, sizeof(struct student));

            // Release the advisory lock on the current student record
            lock.l_type = F_UNLCK;
            if (fcntl(dbFile, F_SETLK, &lock) == -1)
            {
                perror("Error releasing advisory lock on the database record");
                close(dbFile);
                return false;
            }

            recordFound = true;
            break; // No need to continue searching
        }
    }

    // Close the database file
    close(dbFile);

    if (!recordFound)
    {
        return false;
    }

    return true;
}*/
bool unenroll(int clientSocket, struct student *authenticatedStudent)
{
    struct course newCourse;

    // Receive the course details from the client
    send(clientSocket, "Enter course name to unenroll: ", strlen("Enter course name to unenroll: "), 0);
    int readResult = recv(clientSocket, newCourse.name, sizeof(newCourse.name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client", strlen("Error receiving course name from client"), 0);
        return false;
    }
    newCourse.name[readResult] = '\0';

    bool if_enrolled = false;

    // Check if the student is already enrolled in this course
    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            if_enrolled = true;
            break;
        }
    }

    // Open the course database file for reading and writing
    const char *courseDbFileName = "course_db.txt";
    int courseDbFile = open(courseDbFileName, O_RDWR);
    if (courseDbFile == -1)
    {
        perror("Error opening the course database file");
        return false;
    }

    // Define a structure to represent a course
    struct course course_record;
    bool course_found = false;
    if (if_enrolled)
    {
        // Search for the course in the database
        while (read(courseDbFile, &course_record, sizeof(struct course)) > 0)
        {
            if (strcmp(newCourse.name, course_record.name) == 0)
            {
                course_found = true;

                // Decrement the current strength of the course
                course_record.curr_strength--;

                // Update the course record in the database
                lseek(courseDbFile, -sizeof(struct course), SEEK_CUR);
                write(courseDbFile, &course_record, sizeof(struct course));

                break;
            }
        }
    }
    close(courseDbFile);

    if (!course_found)
    {
        send(clientSocket, "Course not found", strlen("Course not found"), 0);
        return false;
    }

    if (!if_enrolled)
    {
        send(clientSocket, "You are not enrolled in this course", strlen("You are not enrolled in this course"), 0);
        return false;
    }

    // Remove the course from the student's list of enrolled courses
    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            // Shift remaining courses to fill the gap
            for (int j = i; j < authenticatedStudent->num_enrolled_courses - 1; j++)
            {
                strcpy(authenticatedStudent->enrolled_courses[j], authenticatedStudent->enrolled_courses[j + 1]);
            }
            // Decrement the number of enrolled courses
            authenticatedStudent->num_enrolled_courses--;
            break;
        }
    }

    // Save the updated student information to the student database file
    if (!save_student_to_db(authenticatedStudent))
    {
        send(clientSocket, "Error saving student information", strlen("Error saving student information"), 0);
        return false;
    }

    send(clientSocket, "Course removal successful", strlen("Course removal successful"), 0);
    return true;
}
void view_enrolled(int clientSocket, struct student *authenticatedstudent)
{
    int i = 0;

    // Check if the professor offers any courses
    if (authenticatedstudent->num_enrolled_courses == 0) {
        send(clientSocket, "You are not enrolled into  any courses.\n", strlen("You are not enrolled into any courses.\n"), 0);
        return;
    }

    // Create a message to display the offered courses
    char courseMessage[1000];
    snprintf(courseMessage, sizeof(courseMessage), "Courses enrolled by you:\n");

    // Send the initial message
    send(clientSocket, courseMessage, strlen(courseMessage), 0);

    // Loop through the offered courses and send them to the client
    for (i = 0; i < authenticatedstudent->num_enrolled_courses; i++) {
        


        char courseName[1000];
        snprintf(courseName, sizeof(courseName), "%s\n", authenticatedstudent->enrolled_courses[i]);

        // Send each course name
        send(clientSocket, courseName, strlen(courseName), 0);
    }
}

bool update_password(int clientSocket, struct student *authenticatedStudent) {
    char newPass[100];

    // Send a prompt for the new password
    const char *passPrompt = "Enter new password: ";
    send(clientSocket, passPrompt, strlen(passPrompt), 0);

    // Receive the new password from the client
    ssize_t bytesRead = recv(clientSocket, newPass, sizeof(newPass), 0);
    if (bytesRead <= 0) {
        close(clientSocket);
        return false;
    }
    if (newPass[bytesRead - 1] == '\n') {
        newPass[bytesRead - 1] = '\0';
    } else {
        newPass[bytesRead] = '\0';
    }

    // Update the student's password
    strncpy(authenticatedStudent->password, newPass, sizeof(authenticatedStudent->password));

    // Save the updated student information to the student database file
    if (!save_student_to_db(authenticatedStudent)) {
        send(clientSocket, "Error saving student information", strlen("Error saving student information"), 0);
        return false;
    }

    send(clientSocket, "Password updated successfully", strlen("Password updated successfully"), 0);
    return true;
}

#endif
