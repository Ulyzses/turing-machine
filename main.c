#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define INIT_STATE_COUNT 8
#define INIT_ALPHABET_SIZE 4
#define HASH_SIZE 64
#define TAPE_SIZE 1024

typedef enum {
    LEFT,
    RIGHT
} MoveDirection;

typedef struct transition_result {
    char *next_state;
    char write_symbol;
    MoveDirection direction;
} TransitionResult;

typedef struct table_entry {
    char *key;
    TransitionResult result;
    struct table_entry *next; // For collisions
} TableEntry;

// Declaring it outside of main so that it can be used by other functions without passing as argument
FILE *f;
char **states;
char *alphabet;
char *tape;
int pointer = 0;
char curr_sym = 0;
char *curr_state = "q_0";
TableEntry **hash_table;

// File parsing util
char* read_line() {
    assert(f != NULL);

    // Usage of calloc to ensure that there is no garbage
    char *line = calloc(MAX_LINE_LENGTH, sizeof(char));

    if (fgets(line, MAX_LINE_LENGTH, f) == NULL) {
        free(line);
        return NULL;
    }

    // Remove trailing newline character if present
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }

    return line;
}

// Hash Map for transition lookup
void make_key(char *dest, const char *state, char symbol) {
    sprintf(dest, "%s,%c", state, symbol);
}

// DJB2 hash function for strings
unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % HASH_SIZE;
}

void add_transition(const char *state, char symbol, const char *next_state, char write_symbol, MoveDirection direction) {
    char key[128];

    make_key(key, state, symbol);

    unsigned long index = hash_function(key);

    TableEntry* entry = hash_table[index];

    // Raise an error if the transition already exists
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            printf("Warning: Overwriting existing transition for key '%s'\n", key);
            fclose(f);
            exit(1);
        }

        entry = entry->next;
    }

    // If no existing entry was found, create a new one
    TableEntry *new_entry = malloc(sizeof(TableEntry));
    new_entry->key = strdup(key);
    new_entry->result.next_state = strdup(next_state);
    new_entry->result.write_symbol = write_symbol;
    new_entry->result.direction = direction;
    new_entry->next = hash_table[index];
    hash_table[index] = new_entry;
}

TransitionResult* get_transition(const char *state, char symbol) {
    char key[128];
    make_key(key, state, symbol);

    unsigned long index = hash_function(key);

    TableEntry* entry = hash_table[index];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return &entry->result;
        }
        entry = entry->next;
    }
    return NULL;
}

void init_file(char *file_name) {
    // Check if the input file exists
    f = fopen(file_name, "r");
    if (f == NULL) {
        printf("Error: Could not open file '%s'\n", file_name);
        fclose(f);
        exit(1);
    }

    // Reading and constructing the machine
    char* buf;

    // Check if the input file is valid
    // printf("Checking if the input file is valid...\n");
    buf = read_line();

    if (buf == NULL) {
        printf("Error: Input file '%s' is empty or invalid\n", file_name);
        free(buf);
        fclose(f);
        exit(1);
    }

    if (strncmp(buf, "tm", 2) != 0) {
        printf("Error: Input file '%s' is not a valid Turing machine description\n", file_name);
        free(buf);
        fclose(f);
        exit(1);
    }

    free(buf);

    // f is freed at the end of main, after parsing the transitions
}

void init_states() {
    char *buf;
    char *token = NULL;
    
    // Initialise the set of states
    // printf("Initialising the set of states...\n");

    // Add the default states
    // printf("Adding default states...\n");
    states = malloc(sizeof(char*) * INIT_STATE_COUNT);
    states[0] = "q_0";
    states[1] = "q_acc";
    states[2] = "q_rej";

    // Parse the states from the input file
    // printf("Parsing the states from the input file...\n");
    int state_count = 3;
    int state_capacity = INIT_STATE_COUNT;

    buf = read_line();

    if (buf != NULL) {
        token = strtok(buf, ",");
    }

    while (token != NULL) {
        // Skip empty tokens
        if (token[0] == '\0' || token[0] == '\n') {
            token = strtok(NULL, ",");
            continue;
        }

        if (state_count >= state_capacity) {
            state_capacity *= 2;
            states = realloc(states, sizeof(char*) * state_capacity);
        }

        states[state_count++] = strdup(token);
        token = strtok(NULL, ",");
    }

    free(buf);
    free(token);

    // for (int i = 0; i < state_count; ++i) {
    //     printf("  State %d: %s\n", i, states[i]);
    // }
}

void init_alphabet() {
    char *buf;
    char *token;

    // Parse the alphabet from the input file
    // printf("Parsing the alphabet from the input file...\n");
    alphabet = malloc(sizeof(char) * INIT_ALPHABET_SIZE);

    int alphabet_size = 0;
    int alphabet_capacity = INIT_ALPHABET_SIZE;

    buf = read_line();

    if (buf == NULL) {
        printf("Alphabet cannot be empty\n");
        free(buf);
        fclose(f);
        exit(1);
    }

    token = strtok(buf, ",");

    while (token != NULL) {
        // Skip empty tokens
        if (token[0] == '\0' || token[0] == '\n') {
            token = strtok(NULL, ",");
            continue;
        }

        if (alphabet_size >= alphabet_capacity) {
            alphabet_capacity *= 2;
            alphabet = realloc(alphabet, sizeof(char) * alphabet_capacity);
        }

        alphabet[alphabet_size++] = token[0];
        token = strtok(NULL, ",");
    }

    free(buf);
    free(token);

    // for (int i = 0; i < alphabet_size; ++i) {
    //     printf("  Symbol %d: %c\n", i, alphabet[i]);
    // }
}

void init_transitions() {
    // Parse the transitions
    char *buf;

    // printf("Parsing the transitions from the input file...\n");

    hash_table = calloc(HASH_SIZE, sizeof(TableEntry*));

    while ((buf = read_line()) != NULL && buf[0] != '\n') {
        char *running = buf;
        char *lhs = strsep(&running, "->");

        if (running && *running == '>') running++;

        char *rhs = strsep(&running, "\n");

        // Process LHS
        char *current_state = strsep(&lhs, ",");
        char read_symbol = lhs[0];

        // Process RHS
        char *next_state = strsep(&rhs, ",");
        char write_symbol = strsep(&rhs, ",")[0];
        char direction_char = rhs[0];
            
        MoveDirection move_direction;

        if (direction_char == 'L') {
            move_direction = LEFT;
        } else if (direction_char == 'R') {
            move_direction = RIGHT;
        } else {
            printf("Error: Invalid move direction '%c' in transition '%s'\n", direction_char, buf);
            free(buf);
            fclose(f);
            exit(1);
        }

        // TODO: Check if the states and symbols are valid (exist in the parsed states and alphabet)

        add_transition(current_state, read_symbol, next_state, write_symbol, move_direction);

        free(buf);
    }

    for (int i = 0; i < HASH_SIZE; ++i) {
        TableEntry* entry = hash_table[i];

        if (entry == NULL) {
            continue;
        }

        // printf("  Hash index %d: ", i);

        // while (entry != NULL) {
        //     printf("\t%s -> %s, %c, %c",
        //         entry->key,
        //         entry->result.next_state,
        //         entry->result.write_symbol,
        //         (entry->result.direction == LEFT) ? 'L' : 'R'
        //     );

        //     entry = entry->next;
        // }

        // printf("\n");
    }
}

// Main Function
int main(int argc, char* argv[]) {
    // Parse arguments
    char *file_name = argv[1];

    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // Initialise the Machine
    init_file(file_name);
    init_states();
    init_alphabet();
    init_transitions();

    fclose(f);

    // Actual Logic
    tape = calloc(TAPE_SIZE, sizeof(char));

    if (argc == 3) {
        strncpy(tape, argv[2], TAPE_SIZE - 1);
    } else {
        printf("Enter the input string for the Turing machine: ");

        fgets(tape, TAPE_SIZE, stdin);
        size_t len = strlen(tape);
        if (len > 0 && tape[len - 1] == '\n') {
            tape[len - 1] = '\0';
        }
    }

    printf("Input: %s\n", tape);

    while (strcmp(curr_state, "q_acc") != 0 && strcmp(curr_state, "q_rej") != 0) {
        // Read the current cell
        curr_sym = *(tape + pointer);

        printf("%s %c -> ", curr_state, (curr_sym) ? curr_sym : '_');

        // Lookup the action
        TransitionResult *result = get_transition(curr_state, curr_sym);

        if (result == NULL) {
            curr_state = "q_rej";
            continue;
        }

        // printf("%s, %c, %c", result->next_state, result->write_symbol, (result->direction == LEFT) ? 'L' : 'R');

        // Write the symbol
        *(tape + pointer) = result->write_symbol;
        curr_state = result->next_state;

        if (result->direction == LEFT) {
            if (pointer != 0) pointer--;
        } else {
            if (pointer != TAPE_SIZE - 1) pointer++;
        }
    }

    printf("%s\n", curr_state);

    if (strcmp(curr_state, "q_acc") == 0) printf("ACCEPT\n");
    if (strcmp(curr_state, "q_rej") == 0) printf("REJECT\n");

    return 0;
}