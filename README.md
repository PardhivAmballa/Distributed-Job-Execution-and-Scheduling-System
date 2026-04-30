# Distributed Job Execution & Scheduling System

## 📌 Overview

This project is a multi-client **client-server system** developed in C that allows users to submit shell commands as jobs. The server schedules and executes these jobs concurrently using operating system concepts such as threading, process creation, inter-process communication, and synchronization.

The system supports **role-based access control**, where:
- Admin can monitor all jobs and view system logs
- Users can submit jobs and view only their own results

---

## 🎯 Features

- Multi-client support using TCP sockets
- Thread pool for handling concurrent client requests
- Job scheduling using a queue (FCFS)
- Separate executor thread for job execution
- Command execution using `fork()` and `exec()`
- Output capture using pipes
- Role-based access control (Admin/User)
- Job ownership restriction
- Logging system with file locking
- Persistent job storage

---

## 🧠 System Architecture
Client → Server → Task Queue → Worker Threads → Job Queue → Executor Thread → Execution → Logging → Result


---

## ⚙️ Technologies & Concepts Used

| Concept | Implementation |
|--------|---------------|
| Socket Programming | TCP client-server communication |
| Multithreading | pthread thread pool |
| Synchronization | Mutex locks, Semaphores |
| Process Management | fork(), exec() |
| IPC | Pipes |
| Scheduling | FCFS queue |
| File Handling | Logging & persistence |
| Security | Role-based access |

---

## 📁 Project Structure
```text
Distributed-Job-Execution-and-Scheduling-System/
│
├── client/
│ └── client.c
│
├── server/
│ ├── server.c
│ ├── queue.c
│ ├── scheduler.c
│ ├── logger.c
│ └── auth.c
│
├── include/
│ ├── common.h
│ ├── queue.h
│ ├── scheduler.h
│ ├── logger.h
│ └── auth.h
│
├── logs/
│ └── system.log
│
├── users.txt
├── Makefile
└── README.md
```

---

## 👥 User Roles

### 👑 Admin
- Submit jobs
- View any job status
- View any job result
- Access system logs

### 👤 User
- Submit jobs
- View own job status
- View own job results
- Cannot access logs or other users’ jobs

---

## 🔄 Workflow

1. Client connects to server via socket
2. User authenticates using `users.txt`
3. Client submits job (command)
4. Server stores job in queue
5. Executor thread picks job (FCFS)
6. Command is executed using fork + exec
7. Output is captured via pipe
8. Result stored and logged
9. Client retrieves status/result

---

## 🛠️ Build & Run

### 🔨 Compile

```bash
make clean
make
```

### ▶️ Run Server
```bash
./server_app
```

### ▶️ Run Client
```bash
./client_app
```

### 🧪 Sample Usage

**Login**
```text
Username: admin
Password: admin123
```

**Submit Job**
```text
1 → Submit Job
Command: ls
```

**Check Status**
```text
2 → Enter Job ID
```

**Get Result**
```text
3 → Enter Job ID
```

### 📜 Logs
Logs are stored in: `logs/system.log`

**Example:**
```text
JobID: 1 | Command: ls | Status: COMPLETED
```

---

## ⚠️ Limitations
- Supports only non-interactive shell commands
- Output limited to buffer size (~1024 chars)
- Fixed queue size (default: 100 jobs)
- Runs on Linux (POSIX system calls)

---

## 🚀 Future Improvements
- Dynamic job queue (linked list)
- Advanced scheduling (Priority, Round Robin)
- Distributed workers (multiple servers)
- Web-based UI
- Database integration

---

## 🧩 Key Learnings
- Handling concurrency using threads and synchronization
- Managing processes using `fork`/`exec`
- Designing modular systems in C
- Implementing real-world client-server architecture
- Understanding scheduling and resource management

---

## 👨‍💻 Author
**Amballa Pardhiv**

---

## 📌 Notes
This project is developed as part of an Operating Systems Lab to demonstrate practical implementation of OS concepts in a real-world system.