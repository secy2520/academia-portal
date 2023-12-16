#ifndef COMMON_H
#define COMMON_H
#define MAX_ENROLLED_COURSES 6
#define MAX_OFFERED_COURSES 10
#define DB_FILE "faculty_db.txt"
#define DATABASE_FILE "student_db.txt"

struct course
{
char name[100];
char course_id[100];
char department[100];
char off_faculty[100];
int curr_strength;
int max_strength;
};


struct student
{
    char name[100];
    char department[100];
    char password[100];
    bool active;
    int employeeID;
    char enrolled_courses[MAX_ENROLLED_COURSES][100];
    int num_enrolled_courses;
};

struct Faculty
{
char name[100];
char department[100];
char password[100];
bool active;
int employeeID;
char offered_courses[MAX_OFFERED_COURSES][100];
int num_offered_courses;
};


bool save_prof_to_db(struct Faculty * authenticatedprof)
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
 /*   while (read(dbaFile, &tempprof, sizeof(struct Faculty)) > 0)
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
}*/

while (read(dbaFile, &tempprof, sizeof(struct Faculty)) > 0)
    {
        if (strcmp(tempprof.name, authenticatedprof->name) == 0)
        {
            // Attempt to obtain an advisory lock on the current student record
            lock.l_start = lseek(dbaFile, -sizeof(struct Faculty), SEEK_CUR);
            lock.l_len = sizeof(struct Faculty);

            if (fcntl(dbaFile, F_SETLK, &lock) == -1)
            {
                perror("Error obtaining advisory lock on the database record\n");
                close(dbaFile);
                return false;
            }

            // Seek back to the beginning of the record and write the updated student data
            if (lseek(dbaFile, -sizeof(struct Faculty), SEEK_CUR) == -1)
            {
                perror("Error seeking to the beginning of the record\n");
                close(dbaFile);
                return false;
            }

            if (write(dbaFile, authenticatedprof, sizeof(struct Faculty)) == -1)
            {
                perror("Error writing the updated student data\n");
                close(dbaFile);
                return false;
            }

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

    close(dbaFile);

    if (!recordFound)
    {
        return false;
    }

    return true;
}

bool save_student_to_db(struct student *student)
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

  close(dbFile);

    if (!recordFound)
    {
        return false;
    }

    return true;

}
#endif
