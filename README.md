# CODAXIS

### A Desktop Git Repository Analytics Application

CODAXIS is a native desktop application built with **C++20 and Qt 6** to explore Git repositories and present repository-level insights through a graphical interface. It uses the Git command-line interface to retrieve repository information and organizes the results through a modular service-based architecture.

The project focuses on making repository information easier to understand while demonstrating object-oriented programming, software modularity, and desktop application development.

---

## Key Features

- **Repository Selection:** Open a local Git repository for analysis.
- **Repository Health Analysis:** Inspect repository information through a dedicated health-analysis service.
- **Git Analytics:** Organize repository analytics through a separate analytics service.
- **Commit and Contributor Insights:** Support analysis of commit history and contributor information.
- **Branch and File Insights:** Examine repository branches and tracked-file information.
- **File Hotspot Analysis:** Identify files with frequent changes to help explore areas of repository activity.
- **Graphical User Interface:** Access repository information through a desktop interface built with Qt Widgets.
- **Error Handling:** Display appropriate feedback when a selected directory is not a valid Git repository.
- **Modular Architecture:** Separate Git command execution, analysis services, and interface responsibilities.

*Feature availability and displayed results depend on the repository and the functionality currently implemented in the application.*

---

## Technology Stack

| Technology | Purpose |
|---|---|
| C++20 | Core application logic and object-oriented design |
| Qt 6 Widgets | Native desktop graphical user interface |
| Qt Creator | Development environment |
| CMake | Build configuration and project management |
| MinGW | C++ compilation toolchain on Windows |
| Git CLI | Repository commands and Git metadata retrieval |
| QProcess | Executing Git commands from the application |

---

## Project Architecture

CODAXIS uses a modular architecture that separates Git command execution, repository analysis, and user-interface responsibilities.

```text
CODAXIS/
├── core/
│   ├── git_client.h
│   ├── git_client.cpp
│   ├── git_service.h
│   ├── repository_health_service.h
│   ├── repository_health_service.cpp
│   ├── git_analytics_service.h
│   ├── git_analytics_service.cpp
│   └── analytics_types.h
├── ui/
│   ├── theme.h
│   ├── theme.cpp
│   ├── repository_canvas.h
│   └── repository_canvas.cpp
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── mainwindow.ui
├── CMakeLists.txt
└── README.md
```

### Core Components

- **GitClient:** Encapsulates Git command execution and collection of command results.
- **GitService:** Defines a common service interface for repository analysis operations.
- **RepositoryHealthService:** Handles repository health checks and exposes their results.
- **GitAnalyticsService:** Provides a separate component for Git analytics.
- **Analytics Types:** Define data structures used to represent analytics results.

### User Interface

The `ui/` directory contains interface-related components, including the visual theme and repository canvas. The main window coordinates the repository workflow and connects the interface to the analysis services.

---

## Object-Oriented Programming Concepts

CODAXIS demonstrates key object-oriented programming concepts:

- **Classes and Objects:** Represent application components and their responsibilities.
- **Encapsulation:** Keep Git command execution and analysis logic within dedicated components.
- **Abstraction:** Define a common `GitService` interface for repository analysis services.
- **Inheritance:** Implement specialized services derived from the common service interface.
- **Runtime Polymorphism:** Use base-class pointers to invoke service operations through the shared interface.

These concepts help keep the application modular and make its components easier to understand and maintain.

---

## Build and Run

### Prerequisites

- A C++ compiler supporting C++20
- Qt 6 with the required Qt Widgets components
- CMake
- Git installed and accessible through the system PATH
- Qt Creator or another compatible CMake development environment

### Build Using Qt Creator

1. Clone or download this repository.
2. Open the project by selecting its `CMakeLists.txt` file in Qt Creator.
3. Select a compatible Qt 6 kit and C++ compiler.
4. Configure the CMake project.
5. Build the project.
6. Run the generated application.

### Build Using CMake

From the project root, configure and build using a compatible Qt 6 kit:

```bash
cmake -S . -B build
cmake --build build
```

The Qt installation and compiler must be configured correctly for CMake to locate Qt and select the appropriate toolchain. The exact launch command depends on the generated executable location and the selected build configuration.

---

## Current Project Status

CODAXIS is under active development.

The current codebase includes the main application entry point, Git command execution, a shared Git service interface, repository health analysis, Git analytics components, and the Qt-based user interface.

Initial manual checks have included opening a sample Git repository, switching to another repository, reopening a repository, and handling an invalid repository directory.

Further development and validation can extend the analytics, interface, testing, and documentation as the project evolves.

---

## Project Goals

CODAXIS aims to combine practical Git repository exploration with a structured C++ application architecture. It serves as an opportunity to apply object-oriented programming, modular software design, process execution, and Qt desktop development in a working project.

---

## License

No license has been specified yet. All rights are reserved by default unless a license is added to this repository.

---

**CODAXIS — Explore repositories. Understand activity.**
