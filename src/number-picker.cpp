#include <cstdlib>  // For getenv, rand
#include <iostream> // For cout, cerr, endl
#include <stdlib.h> // For atoi
#include <unistd.h> // For sleep

const int DEFAULT_SLEEP_INTERVAL = 2;
const char* SLEEP_INTERVAL_ENV_VARIABLE_NAME = "NUMBER_PICKER_INTERVAL";

int pickNumber() {
    int number = rand();
    std::cout << "My number is " << number << std::endl;
    return number;
}

void run(int sleepInterval) {
    while (true) {
        int number = pickNumber();
        sleep(sleepInterval);
    }
}

int main(int argc, const char* argv[]) {
    int sleepInterval = DEFAULT_SLEEP_INTERVAL;

    char* envVariableValue = getenv(SLEEP_INTERVAL_ENV_VARIABLE_NAME);
    if (envVariableValue != NULL) {
        sleepInterval = atoi(envVariableValue);
    }

    if (argc > 1) {
        sleepInterval = atoi(argv[1]);
    }

    if (sleepInterval == 0) {
        std::cerr << "Invalid sleep interval: " << argv[1] << std::endl;
        return 1;
    }
    run(sleepInterval);
}
