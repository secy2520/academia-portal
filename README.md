# Academia Portal

Welcome to the Academia Portal project! Developed by me during my MTech, this user-friendly and multifunctional system is designed for efficient management of student and faculty information, along with course details, using C programming. Below, you'll find an overview of the key features and functionalities.

## Features

- **Data Storage**: All student, faculty, and course information is efficiently stored in files using the power of C programming for seamless access and management.

- **User Authentication**: Account holders, including Faculty, Students, and Administrators, need to pass through a secure login system managed by the Administrator.

- **Role-Based Access**: The system incorporates distinct roles for Faculty, Students, and Admin, each with tailored functionalities.

- **Administrative Access Control**: The application ensures password-protected administrative access, preventing unauthorized entry and ensuring the security of the entire management system.

- **Menu-driven Interface**:
  - *Admin Menu*: After successful login, the Admin is presented with a menu offering actions such as adding students or faculty, activating/deactivating students, updating details, and more.
  - *Student Menu*: Students, upon login, access a menu for actions like enrolling in new courses, unenrolling from existing courses, viewing enrolled courses, changing passwords, and more.
  - *Faculty Menu*: Faculty members, post-login, have a menu offering options like adding new courses, removing offered courses, viewing enrollments in courses, changing passwords, and more.

- **Data Integrity through Locking Mechanisms**: The project implements read and write locks in C programming to ensure data integrity. Read locks are set for viewing course details, while write locks protect critical data sections during enrollment or unenrollment processes.

- **Socket Programming**: The server utilizes socket programming to maintain a centralized database, allowing multiple clients to connect concurrently. Clients can access their specific academic details seamlessly.

- **System Calls Implementation**: The project prioritizes system calls over library functions wherever possible in processes like Process Management, File Management, File Locking, Semaphores, Multithreading, and Inter-Process Communication Mechanisms.


