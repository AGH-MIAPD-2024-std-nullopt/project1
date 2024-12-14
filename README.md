# Running Docker for the Project

To get started with Docker for this project, follow the instructions below:

## Prerequisites

1. **Docker Installed**: Make sure you have Docker installed on your system. You can download it from [Docker's official website](https://www.docker.com/products/docker-desktop).
2. **WSL 2**: If you are on Windows, ensure that Windows Subsystem for Linux (WSL) 2 is enabled.

## Steps to Run Docker

1. **Open Terminal**: Open your terminal (Command Prompt, PowerShell, or your WSL terminal).
2. **Navigate to Project Directory**: Use the `cd` command to navigate to the root directory of your project where the `Dockerfile` is located.
   ```bash
   cd path/to/project
   ```
3. **Build the Docker Image**: Run the following command to build the Docker image.
   ```bash
   docker build -t ahpwebserver .
   ```
4. **Run the Docker Container**: After building the image, you can run the container using the following command.
   ```bash
   docker run -p 8080:8080 ahpwebserver
   ```
6. **Stop and Remove the Container**: To stop the running container, use:
   ```bash
   docker stop ahpwebserver
   ```
   To remove the container after stopping it, use:
   ```bash
   docker rm ahpwebserver
   ```
7. **Clean Up**: To remove the Docker image if needed, use:
   ```bash
   docker rmi ahpwebserver
   ```

Alternatively you can use 'docker compose up --force-recreate' in main catalog

## Additional Information

For more details on using Docker, refer to the [Docker documentation](https://docs.docker.com/).

