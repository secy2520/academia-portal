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

#define MAX_ENROLLED_COURSES 6
#define DATABASE_FILE "student_db.txt"

bool authenticate_stud(int clientSocket, struct student *authenticatedStudent);
bool enrollments(int clientSocket, struct student *authenticatedStudent);
bool save_student_to_db(struct student *student);
bool unenroll(int clientSocket, struct student *authenticatedStudent);
void view_enrolled(int clientSocket, struct student *authenticatedStudent);

bool stud_operation_handler(int clientSocket)
{
    struct student authenticatedStudent;

    bool authenticated = authenticate_stud(clientSocket, &authenticatedStudent);
    if (!authenticated)
    {
        return false;
    }

    while (1)
    {
        char studentMenu[] = "Do you want to:\n - 1. Enroll in new courses\n - 2. Unenroll from already enrolled courses\n - 3. View enrolled courses\n - 4. Exit\n";
        send(clientSocket, studentMenu, strlen(studentMenu), 0);

        char readbuff[1000];
        ssize_t readBytes = recv(clientSocket, readbuff, sizeof(readbuff), 0);
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
                send(clientSocket, "There is some error. Please try again!\n", strlen("There is some error. Please try again!\n"), 0);
            }
            break;
        case 2:
            if (unenroll(clientSocket, &authenticatedStudent))
            {
                send(clientSocket, "Done successfully\n", strlen("Done successfully\n"), 0);
            }
            else
            {
                send(clientSocket, "There is some error. Please try again!\n", strlen("There is some error. Please try again!\n"), 0);
            }
            break;
        case 3:
            view_enrolled(clientSocket, &authenticatedStudent);
            break;
        case 4:
            close(clientSocket);
            return true;
        }
    }

    return false;
}

bool authenticate_stud(int clientSocket, struct student *authenticatedStudent)
{
    char username[100];
    char pass[100];

    const char *userPrompt = "Enter username: ";
    send(clientSocket, userPrompt, strlen(userPrompt), 0);

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

    const char *passPrompt = "Enter password: ";
    send(clientSocket, passPrompt, strlen(passPrompt), 0);

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

    int dbFile = open(DATABASE_FILE, O_RDWR | O_CREAT, 0666);
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

    while (read(dbFile, &stud, sizeof(struct student)) > 0)
    {
        if (found == true)
            break;
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
            *authenticatedStudent = stud;
            found = true;
        }

        lock.l_type = F_UNLCK;
        if (fcntl(dbFile, F_SETLK, &lock) == -1)
        {
            perror("Error releasing advisory lock on the database record");
            close(dbFile);
            close(clientSocket);
            return false;
        }
    }

    close(dbFile);

    if (found)
    {
        const char *successMessage = "Authentication successful";
        send(clientSocket, successMessage, strlen(successMessage), 0);
        return true;
    }
    else
    {
        const char *failureMessage = "Authentication failed";
        send(clientSocket, failureMessage, strlen(failureMessage), 0);
        close(clientSocket);
        return false;
    }
}

bool enrollments(int clientSocket, struct student *authenticatedStudent)
{
    struct course newCourse;

    send(clientSocket, "Enter course name: ", strlen("Enter course name: "), 0);
    ssize_t readResult = recv(clientSocket, newCourse.name, sizeof(newCourse.name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client", strlen("Error receiving course name from client"), 0);
        return false;
    }
    newCourse.name[readResult] = '\0';

    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            send(clientSocket, "You are already enrolled in this course", strlen("You are already enrolled in this course"), 0);
            return false;
        }
    }

    const char *courseDbFileName = "course_db.txt";
    int courseDbFile = open(courseDbFileName, O_RDWR);
    if (courseDbFile == -1)
    {
        perror("Error opening the course database file");
        return false;
    }

    struct course course_record;
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
            }

            course_record.curr_strength++;

            lseek(courseDbFile, -sizeof(struct course), SEEK_CUR);
            write(courseDbFile, &course_record, sizeof(struct course);

            break;
        }
    }

    close(courseDbFile);

    if (!course_found)
    {
        send(clientSocket, "Course not found", strlen("Course not found"), 0);
        return false;
    }

    if (authenticatedStudent->num_enrolled_courses < MAX_ENROLLED_COURSES)
    {
        strcpy(authenticatedStudent->enrolled_courses[authenticatedStudent->num_enrolled_courses], newCourse.name);
        authenticatedStudent->num_enrolled_courses++;

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
}

bool save_student_to_db(struct student *student)
{
    int dbFile = open(DATABASE_FILE, O_RDWR | O_CREAT, 0666);
    if (dbFile == -1)
    {
        if (errno == EEXIST)
        {
            dbFile = open(DATABASE_FILE, O_RDWR);
        }
        else
        {
            perror("Error opening the database file");
            return false;
        }
    }

    struct student tempStudent;
    bool recordFound = false;
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;

    while (read(dbFile, &tempStudent, sizeof(struct student)) > 0)
    {
        if (strcmp(tempStudent.name, student->name) == 0)
        {
            lock.l_start = lseek(dbFile, 0, SEEK_CUR) - sizeof(struct student);
            lock.l_len = sizeof(struct student);
            if (fcntl(dbFile, F_SETLK, &lock) == -1)
            {
                perror("Error obtaining advisory lock on the database record");
                close(dbFile);
                return false;
            }

            lseek(dbFile, -sizeof(struct student), SEEK_CUR);
            write(dbFile, student, sizeof(struct student));

            lock.l_type = F_UNLCK;
            if (fcntl(dbFile, F_SETLK, &lock) == -1)
            {
                perror("Error releasing advisory lock on the database record");
                close(dbFile);
                return false;
            }

            recordFound = true;
            break;
        }
    }

    close(dbFile);

    if (!recordFound)
    {
        return false;
    }

    return true;
}

bool unenroll(int clientSocket, struct student *authenticatedStudent)
{
    struct course newCourse;

    send(clientSocket, "Enter course name to unenroll: ", strlen("Enter course name to unenroll: "), 0);
    int readResult = recv(clientSocket, newCourse.name, sizeof(newCourse.name), 0);
    if (readResult <= 0)
    {
        send(clientSocket, "Error receiving course name from client", strlen("Error receiving course name from client"), 0);
        return false;
    }
    newCourse.name[readResult] = '\0;

    bool if_enrolled = false;

    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            if_enrolled = true;
            break;
        }
    }

    const char *courseDbFileName = "course_db.txt";
    int courseDbFile = open(courseDbFileName, O_RDWR);
    if (courseDbFile == -1)
    {
        perror("Error opening the course database file");
        return false;
    }

    struct course course_record;
    bool course_found = false;

    if (if_enrolled)
    {
        while (read(courseDbFile, &course_record, sizeof(struct course)) > 0)
        {
            if (strcmp(newCourse.name, course_record.name) == 0)
            {
                course_found = true;
                course_record.curr_strength--;

                lseek(courseDbFile, -sizeof(struct course), SEEK_CUR);
                write(courseDbFile, &course_record, sizeof(struct course);

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

    for (int i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        if (strcmp(authenticatedStudent->enrolled_courses[i], newCourse.name) == 0)
        {
            for (int j = i; j < authenticatedStudent->num_enrolled_courses - 1; j++)
            {
                strcpy(authenticatedStudent->enrolled_courses[j], authenticatedStudent->enrolled_courses[j + 1]);
            }
            authenticatedStudent->num_enrolled_courses--;
            break;
        }
    }

    if (!save_student_to_db(authenticatedStudent))
    {
        send(clientSocket, "Error saving student information", strlen("Error saving student information"), 0);
        return false;
    }

    send(clientSocket, "Course removal successful", strlen("Course removal successful"), 0);
    return true;
}

void view_enrolled(int clientSocket, struct student *authenticatedStudent)
{
    int i = 0;

    if (authenticatedStudent->num_enrolled_courses == 0)
    {
        send(clientSocket, "You are not enrolled into any courses.\n", strlen("You are not enrolled into any courses.\n"), 0);
        return;
    }

    char courseMessage[1000];
    snprintf(courseMessage, sizeof(courseMessage), "Courses enrolled by you:\n");

    send(clientSocket, courseMessage, strlen(courseMessage), 0);

    for (i = 0; i < authenticatedStudent->num_enrolled_courses; i++)
    {
        char courseName[1000];
        snprintf(courseName, sizeof(courseName), "%s\n", authenticatedStudent->enrolled_courses[i]);
        send(clientSocket, courseName, strlen(courseName), 0);
    }
}

#endif
