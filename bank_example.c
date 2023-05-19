#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>

#include "mymutex.h"

#define MAX_PATH_LENGTH 1024

#define err_exit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

// Main Function

void deposit(const char *bankfile, float amount);
void withdraw(const char *bankfile, float amount);
void balance(const char *bankfile);

int main(int argc, char *argv[]) {
    int fd;
    int choice;
    float amount;
    char *bankfile  = "./bankfile.bin";

// Check for correct number of command - line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <host_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//Check if bankfile exists, create it if not

    if (access(bankfile, F_OK) == -1) {
        if ((fd = open(bankfile, O_CREAT | O_WRONLY, 0666)) == -1)
            err_exit(bankfile);
        amount = 0.0;
        if (write(fd, &amount, sizeof(float)) == -1)
            err_exit(bankfile);
        close(fd);
    }

//Initialize the mutex
    mymutex_init(atoi(argv[1]));

//Displaying the available operations
    printf("Available operations:\n");
    printf(" 1.Deposit\n"
           " 2.Withdraw\n"
           " 3.Balance\n"
           " 4.Exit\n");
//Main loop for user input
    while (1) {
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                printf("Enter amount to deposit: ");
                scanf("%f", &amount);
                deposit(bankfile, amount);
                break;
            case 2:
                printf("Enter amount to withdraw: ");
                scanf("%f", &amount);
                withdraw(bankfile, amount);
                break;
            case 3:
                balance(bankfile);
                break;
            case 4:
                mymutex_destroy();
                exit(EXIT_SUCCESS);
            default:
                printf("Invalid choice\n");
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
        }
    }
}
//Desposit function
void deposit(const char *bankfile, float amount) {
    int fd;
    float current_amount;
	// Validate the deposit amount
    if (amount <= 0.0) {
        printf("Cannot deposit $%.2f\n", amount);
        return;
    }

//Acquiring the mutex lock
    mymutex_acquire_lock();

        sleep(3);
//Open the bank file for reading and writing

    if ((fd = open(bankfile, O_RDWR | O_SYNC)) == -1)
        err_exit("open");
// Read the cureent balance
    if (read(fd, &current_amount, sizeof(float)) == -1)
        err_exit("read");
// Update the balance 
    current_amount += amount;
    if (lseek(fd, 0, SEEK_SET) == -1)
        err_exit("lseek");
// Writing the new balance
    if (write(fd, &current_amount, sizeof(float)) == -1)
        err_exit("write");
    close(fd);

    mymutex_release_lock();

    printf("Deposited $%.2f, new balance is $%.2f\n", amount, current_amount);

}
// Withdraw function 
void withdraw(const char *bankfile, float amount) {
    int fd; // File descriptor for the bank account file
    float current_amount; // Stores the current balance of the account

    if (amount <= 0.0) {
        printf("Cannot withdraw $%.2f\n", amount);
        return;
    }

// Acquire a mutex lock to prevent concurrent access to the bank account file.
    mymutex_acquire_lock();
    sleep(3); //delaying

    if ((fd = open(bankfile, O_RDWR | O_SYNC)) == -1)
        err_exit("open");
    if (read(fd, &current_amount, sizeof(float)) == -1)
        err_exit("read");
    if (amount > current_amount) {
        printf("Cannot withdraw $%.2f, current balance is $%.2f\n", amount, current_amount);
        mymutex_release_lock();
        close(fd);
        return;
    }
    if (lseek(fd, 0, SEEK_SET) == -1)
        err_exit("lseek");
    current_amount -= amount;
    if (write(fd, &current_amount, sizeof(float)) == -1)
        err_exit("write");
    close(fd);

    mymutex_release_lock();

    printf("Withdrew $%.2f, new balance is $%.2f\n", amount, current_amount);
}
// Balance Check
void balance(const char *bankfile) {
    int fd;
    float current_amount;

    mymutex_acquire_lock();
    sleep(3);

    if ((fd = open(bankfile, O_RDONLY)) == -1)
        err_exit("open");
    if (read(fd, &current_amount, sizeof(float)) == -1)
        err_exit("read");
    close(fd);

    mymutex_release_lock(); // releasing the lock 

    printf("Current balance is $%.2f\n", current_amount);
}
