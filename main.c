#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define LENGTH_OF_ACCOUNT_NUMBER 10
#define LENGTH_OF_NAME 16
#define LENGTH_OF_SURNAME 16
#define LENGTH_OF_ADDRESS 32
#define LENGTH_OF_NATIONAL_ID 11
#define LENGTH_OF_BALANCE 10
#define LENGTH_OF_LOAN_BALANCE 10
#define LENGTH_OF_INTEREST_RATE 7

#define FULL_VIEW 1
#define SHORT_VIEW 2

#define RECORD_FILE "records.txt"
#define WELCOME_SCREEN_FILE "welcome_screen.txt"

#define MAX_COMMAND_LENGTH 64
#define N_OF_COMMANDS 14

const char* COMMANDS[] = {
        "list",
        "add",
        "deposit",
        "withdraw",
        "borrow",
        "repay",
        "transfer",
        "search",
        "get",
        "quit",
        "reset_file",
        "collect_interest",
        "help",
        "paste"
};

typedef struct Account{
    uint32_t account_number; // specifications call for 'unlimited' number of accounts
    char name[LENGTH_OF_NAME];           // but just 2^32 records will not fit on any even remotely reasonable storage (as of 2024)
    char surname[LENGTH_OF_SURNAME];
    char address[LENGTH_OF_ADDRESS];
    char national_id[LENGTH_OF_NATIONAL_ID]; // PESEL
    int32_t curr_balance;
    int32_t loan_balance;
    double interest_rate;
} acc_t;


#define MAX_DEPOSIT 100000
#define MAX_WITHDRAW 10000
#define MAX_BORROW 1000000
#define MAX_TRANSFER MAX_DEPOSIT
#define MAX_ACCOUNT_VALUE (INT32_MAX - MAX_DEPOSIT)
#define MAX_LOAN_VALUE (INT32_MAX - MAX_BORROW)

const acc_t NULL_ACCOUNT = {0, "", "", "", "", 0, 0, 0};
const acc_t ROOT_BANK_ACCOUNT = {1, "Bank", "Bank", "ul. Bankowa 1 00-001 Warszawa", "00000000000", INT32_MAX/2, 0, 0};

const acc_t PRESET_ACCOUNTS[] = {
        1, "Jan", "Kowalski", "ul. Kowalska 1 00-001 Warszawa", "12345678901", 1000, 0, 0.1,
        2, "Anna", "Nowak", "ul. Nowa 1 00-001 Warszawa", "12345678902", 2000, 0, 0.2,
        3, "Piotr", "Kowalczyk", "ul. Kolczykowa 1 00-001 Warszawa", "12345678903", 3000, 0, 0.3,
        4, "Agnieszka", "Kowalska", "ul. Kowalska 2 00-001 Warszawa", "12345678904", 4000, 0, 0.4,
        5, "Janusz", "Nowak", "ul. Nowa 2 00-001 Warszawa", "12345678905", 5000, 0, 0.5,
        6, "Krzysztof", "Kowalczyk", "ul. Kolczowa 2 00-001 Warszawa", "12345678906", 6000, 0, 0.6,
        7, "Alicja", "Kowalska", "ul. Kowalska 3 00-001 Warszawa", "12345678907", 7000, 0, 0.7,
        8, "Jan", "Nowak", "ul. Nowa 3 00-001 Warszawa", "12345678908", 8000, 0, 0.8,
        9, "Anna", "Kowalczyk", "ul. Kowalkowa 3 00-001 Warszawa", "12345678909", 9000, 0, 0.9,
        10, "Piotr", "Kowalski", "ul. Kowalska 4 00-001 Warszawa", "12345678910", 10000, 0, 0.1
};

bool REQUIRE_CONFIRMATION_ON_EDIT = true;
int global_view_mode = FULL_VIEW;
uint32_t number_of_accounts = 0;

void print_help(){
    printf("Available commands:\n");
    printf("list <view_mode> - list all accounts 0-full_view 1-short_view\n");
    printf("add - add new account\n");
    printf("deposit <account_number> <value> - deposit money to account\n");
    printf("withdraw <account_number> <value> - withdraw money from account\n");
    printf("borrow <account_number> <value> - borrow money from bank\n");
    printf("repay <account_number> <value> - repay loan\n");
    printf("transfer <origin_account_number> <dest_account_number2> <amount>\n");
    printf("search <search option> <searched value>- search for account\n");
    printf("get <account_number> - get account info\n");
    printf("quit - exit program\n");
    printf("reset_file - reset file to initial state\n");
    printf("collect_interest <account_number> - collect interest on loan\n");
    printf("help - display this message\n");
}

void print_search_help(){
    printf("Search options:\n");
    printf("0 - search by account number\n");
    printf("1 - search by name\n");
    printf("2 - search by surname\n");
    printf("3 - search by address\n");
    printf("4 - search by national ID\n");
}

void print_welcome_screen() {
    FILE *file = fopen(WELCOME_SCREEN_FILE, "r");
    if (file == NULL) {
        printf("Error opening welcome file\n");
        return;
    }
    while (1) {
        int c = fgetc(file);
        if (c == EOF) {
            break;
        }
        printf("%c", c);
    }
    printf("\n");
    fclose(file);
}

void print_table_header(int view_mode){
    printf("| %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s |\n",
           LENGTH_OF_ACCOUNT_NUMBER/view_mode, LENGTH_OF_ACCOUNT_NUMBER/view_mode, "Account",
           LENGTH_OF_NAME/view_mode, LENGTH_OF_NAME/view_mode, "Name",
           LENGTH_OF_SURNAME/view_mode, LENGTH_OF_SURNAME/view_mode, "Surname",
           LENGTH_OF_ADDRESS/view_mode, LENGTH_OF_ADDRESS/view_mode, "Address",
           LENGTH_OF_NATIONAL_ID/view_mode, LENGTH_OF_NATIONAL_ID/view_mode, "National ID",
           LENGTH_OF_BALANCE, LENGTH_OF_BALANCE, "Balance",
           LENGTH_OF_LOAN_BALANCE, LENGTH_OF_LOAN_BALANCE, "Loan",
           LENGTH_OF_INTEREST_RATE, LENGTH_OF_INTEREST_RATE+2, "Intre");
    printf("| %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s |\n",
           LENGTH_OF_ACCOUNT_NUMBER/view_mode, LENGTH_OF_ACCOUNT_NUMBER/view_mode, "-----------",
           LENGTH_OF_NAME/view_mode, LENGTH_OF_NAME/view_mode, "-----------------",
           LENGTH_OF_SURNAME/view_mode, LENGTH_OF_SURNAME/view_mode, "-----------------",
           LENGTH_OF_ADDRESS/view_mode, LENGTH_OF_ADDRESS/view_mode, "------------------------------------------------",
           LENGTH_OF_NATIONAL_ID/view_mode, LENGTH_OF_NATIONAL_ID/view_mode, "-----------",
           LENGTH_OF_BALANCE, LENGTH_OF_BALANCE, "-----------",
           LENGTH_OF_LOAN_BALANCE, LENGTH_OF_LOAN_BALANCE, "-----------",
           LENGTH_OF_INTEREST_RATE, LENGTH_OF_INTEREST_RATE+2, "-----------");
}

void print_account_as_table(acc_t account, int view_mode){
    printf("| %0*.*u | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.d | %-*.d | %0.*f |\n",
           LENGTH_OF_ACCOUNT_NUMBER/view_mode, LENGTH_OF_ACCOUNT_NUMBER/view_mode, account.account_number,
           LENGTH_OF_NAME/view_mode, LENGTH_OF_NAME/view_mode, account.name,
           LENGTH_OF_SURNAME/view_mode, LENGTH_OF_SURNAME/view_mode, account.surname,
           LENGTH_OF_ADDRESS/view_mode, LENGTH_OF_ADDRESS/view_mode, account.address,
           LENGTH_OF_NATIONAL_ID/view_mode, LENGTH_OF_NATIONAL_ID/view_mode, account.national_id,
           LENGTH_OF_BALANCE, account.curr_balance,
           LENGTH_OF_LOAN_BALANCE, account.loan_balance,
           LENGTH_OF_INTEREST_RATE, account.interest_rate);
}

int read_all_records(int view_mode){
    if(view_mode != FULL_VIEW && view_mode != SHORT_VIEW){
        printf("Invalid view mode\n");
        return 1;
    }
    FILE *file = fopen(RECORD_FILE, "r");
    if (file == NULL){
        printf("Error opening file\n");
        return 1;
    }
    acc_t acc;
    print_table_header(view_mode);
    while (fread(&acc, sizeof(acc_t), 1, file)){
        print_account_as_table(acc, view_mode);
    }
    fclose(file);
    return 0;
}

int is_account_null(acc_t account) {
    if (account.account_number == NULL_ACCOUNT.account_number &&
        account.curr_balance == NULL_ACCOUNT.curr_balance &&
        account.loan_balance == NULL_ACCOUNT.loan_balance &&
        account.interest_rate == NULL_ACCOUNT.interest_rate &&
        strncmp(account.name, NULL_ACCOUNT.name, LENGTH_OF_NAME) == 0 &&
        strncmp(account.surname, NULL_ACCOUNT.surname, LENGTH_OF_SURNAME) == 0 &&
        strncmp(account.address, NULL_ACCOUNT.address, LENGTH_OF_ADDRESS) == 0 &&
        strncmp(account.national_id, NULL_ACCOUNT.national_id, LENGTH_OF_NATIONAL_ID) == 0)
        return true;
    return false;
}

int verify_account_validity(acc_t account){
    if (account.account_number == NULL_ACCOUNT.account_number)
        return 1;
    if (account.curr_balance < 0 || account.curr_balance > MAX_ACCOUNT_VALUE)
        return 2;
    if (account.loan_balance < 0 || account.loan_balance > MAX_LOAN_VALUE)
        return 3;
    if (account.interest_rate < 0 || account.interest_rate > 1)
        return 4;
    return 0;
}

int get_confirmation(){
    char confirmation;
    printf("Are you sure you want to make changes to the record file? (y/n)\n");
    scanf("%c", &confirmation);
    while (getchar() != '\n');
    if (confirmation == 'y')
        return true;
    else
        return false;
}

int reset_file() {
    printf("resetting file\n");
    if(get_confirmation() == false)
        return 1;
    FILE *file = fopen(RECORD_FILE, "w");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    fwrite(&NULL_ACCOUNT, sizeof(acc_t), 1, file);
    fwrite(&ROOT_BANK_ACCOUNT, sizeof(acc_t), 1, file);
    fclose(file);
    number_of_accounts = 1;
    return 0;
}

int verify_file_integrity(){
    FILE *file = fopen(RECORD_FILE, "r");
    if (file == NULL){
        printf("Error opening file\n");
        return 1;
    }
    acc_t account;
    while (fread(&account, sizeof(acc_t), 1, file)){
        if (verify_account_validity(account) != 0 && is_account_null(account)==false){
            print_table_header(FULL_VIEW);
            print_account_as_table(account, FULL_VIEW);
            printf("Error verifying file integrity - reset_file or manual trimming of data advised\n");
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

acc_t get_account(uint32_t account_number){
    if (account_number == 0 || account_number > number_of_accounts){
        printf("Invalid account number\n");
        return NULL_ACCOUNT;
    }
    FILE *file = fopen(RECORD_FILE, "r");
    if (file == NULL){
        printf("Error opening file\n");
        return NULL_ACCOUNT;
    }
    if (fseek(file, account_number * sizeof(acc_t), SEEK_SET) != 0) {
        printf("Error finding account - id possibly out of range\n");
        fclose(file);
        return NULL_ACCOUNT;
    }
    acc_t account;
    fread(&account, sizeof(acc_t), 1, file);
    fclose(file);
    return account;
}

acc_t get_last_account(){
    FILE *file = fopen(RECORD_FILE, "r");
    if (file == NULL){
        printf("Error opening file\n");
        return NULL_ACCOUNT;
    }
    fseek(file, -(long)sizeof(acc_t), SEEK_END); // go to the last record (assuming it's there
    acc_t last_account = NULL_ACCOUNT;
    if(fread(&last_account, sizeof(acc_t), 1, file) == 0){
        printf("Error reading last account\n");
        fclose(file);
        return last_account;
    }
    if (number_of_accounts != 0 && last_account.account_number != number_of_accounts){
        printf("Error reading last account - acc numbers not in sync\n");
    }
    fclose(file);
    return last_account;
}

int add_account(acc_t new_account) {
    acc_t last_account = get_last_account();
    new_account.account_number = last_account.account_number + 1;
    FILE *file = fopen(RECORD_FILE, "a");
    if (verify_account_validity(new_account) != 0){
        printf("Error adding account - invalid data\n");
        return 1;
    }
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    fwrite(&new_account, sizeof(acc_t), 1, file);
    number_of_accounts++;
    fclose(file);
    return 0;
}

int populate_file_with_preset_accounts(){
    for (int i = 0; i < sizeof(PRESET_ACCOUNTS)/sizeof(acc_t); i++){
        if(add_account(PRESET_ACCOUNTS[i]) != 0)
            return 1;
    }
    return 0;
}

int convert_newlines_to_whitespace(char* string, int length){
    for (int i = 0; i < length; i++){
        if (string[i] == '\n')
            string[i] = ' ';
        else if (string[i] == '\0')
            return 1;
    }
    return 0;
}

int get_and_clean_input(char* input_buffer, int buffer_size) {
    fgets(input_buffer, buffer_size, stdin);
    int len = strlen(input_buffer);
    if (len == 0){
        return 1;
    } else if (len >= buffer_size - 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    convert_newlines_to_whitespace(input_buffer, len);
    return 0;
}

int add_account_from_input(){
    acc_t new_account;
    char temp_balance[LENGTH_OF_BALANCE], temp_loan_balance[LENGTH_OF_LOAN_BALANCE], temp_interest_rate[LENGTH_OF_INTEREST_RATE];
    printf("Enter name: ");
    get_and_clean_input(new_account.name, LENGTH_OF_NAME);
    printf("Enter surname: ");
    get_and_clean_input(new_account.surname, LENGTH_OF_SURNAME);
    printf("Enter address: ");
    get_and_clean_input(new_account.address, LENGTH_OF_ADDRESS);
    printf("Enter national ID: ");
    get_and_clean_input(new_account.national_id, LENGTH_OF_NATIONAL_ID);
    printf("Enter current balance: ");
    get_and_clean_input(temp_balance, LENGTH_OF_BALANCE);
    new_account.curr_balance = strtol(temp_balance, NULL, 10);
    printf("Enter loan balance: ");
    get_and_clean_input(temp_loan_balance, LENGTH_OF_LOAN_BALANCE);
    new_account.loan_balance = strtol(temp_loan_balance, NULL, 10);
    printf("Enter interest rate: ");
    get_and_clean_input(temp_interest_rate, LENGTH_OF_INTEREST_RATE);
    new_account.interest_rate = strtod(temp_interest_rate, NULL);
    printf("Account to be added:\n");
    print_table_header(global_view_mode);
    print_account_as_table(new_account, global_view_mode);
    if (get_confirmation() == false || add_account(new_account)==1){
        return 1;
    }
    printf("Account added successfully\n");

    return 0;
}

int paste_account_at_number(uint32_t account_number, acc_t new_account) {
    if(REQUIRE_CONFIRMATION_ON_EDIT && get_confirmation()==false){
        printf("Operation aborted\n");
        return 1;
    }
    if(verify_account_validity(new_account) != 0){
        printf("Error updating account - invalid data\n");
        return 1;
    }
    FILE *file = fopen(RECORD_FILE, "r+");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    if (fseek(file, account_number * sizeof(acc_t), SEEK_SET) != 0) {
        printf("Error finding account - id possibly out of range\n");
        fclose(file);
        return 1;
    }
    new_account.account_number = account_number;
    fwrite(&new_account, sizeof(acc_t), 1, file);
    fclose(file);
    return 0;
}

int make_deposit(uint32_t account_number, int32_t deposit_value){
    if (deposit_value <= 0){
        printf("Deposit value must be positive\n");
        return 1;
    }
    if (deposit_value > MAX_DEPOSIT){
        printf("Deposit value exceeds maximum deposit value (%d)\n", MAX_DEPOSIT);
        return 1;
    }
    acc_t account = get_account(account_number);
    if (account.account_number == NULL_ACCOUNT.account_number){
        printf("Account not found\n");
        return 1;
    }
    if(account.curr_balance + deposit_value > MAX_ACCOUNT_VALUE){
        printf("Deposit exceeds maximum account value (%d)\n", MAX_ACCOUNT_VALUE);
        return 1;
    }
    account.curr_balance += deposit_value;
    if (paste_account_at_number(account_number, account)==1)
        return 1;
    else
        return 0;
}

int make_withdraw(uint32_t account_number, int32_t withdraw_value){
    if (withdraw_value <= 0){
        printf("Withdraw value must be positive\n");
        return 1;
    }
    if (withdraw_value > MAX_WITHDRAW){
        printf("Withdraw value exceeds maximum withdraw value (%d)\n", MAX_WITHDRAW);
        return 1;
    }
    acc_t account = get_account(account_number);
    if (account.account_number == NULL_ACCOUNT.account_number){
        printf("Account not found\n");
        return 1;
    }
    if(account.curr_balance - withdraw_value < 0){
        printf("Withdraw exceeds account balance\n");
        return 1;
    }
    account.curr_balance -= withdraw_value;
    if (paste_account_at_number(account_number, account)==1)
        return 1;
    else
        return 0;
}

int take_loan(uint32_t account_number, int32_t loan_value){
    if (loan_value <= 0){
        printf("Loan value must be positive\n");
        return 1;
    }
    if (loan_value > MAX_BORROW){
        printf("Loan value exceeds maximum loan value (%d)\n", MAX_BORROW);
        return 1;
    }
    acc_t account = get_account(account_number);
    if (account.account_number == NULL_ACCOUNT.account_number){
        printf("Account not found\n");
        return 1;
    }
    if (account.curr_balance + loan_value > MAX_ACCOUNT_VALUE){
        printf("New account value exceeds max account value (%d)\n", MAX_ACCOUNT_VALUE);
        return 1;
    }
    if(account.loan_balance + loan_value > MAX_LOAN_VALUE){
        printf("Loan exceeds maximum loan value (%d)\n", MAX_LOAN_VALUE);
        return 1;
    }
    acc_t bank_account = get_account(ROOT_BANK_ACCOUNT.account_number);
    if (loan_value > bank_account.curr_balance){
        printf("Bank does not have enough funds to provide loan\n");
        return 1;
    }
    bank_account.curr_balance -= loan_value;
    account.loan_balance += loan_value;
    account.curr_balance += loan_value;
    if (paste_account_at_number(account_number, account)==1 ||
        paste_account_at_number(ROOT_BANK_ACCOUNT.account_number, bank_account)==1)
        return 1;
    else
        return 0;
}

int repay_loan(uint32_t account_number, int32_t payment_value){
    if (payment_value <= 0){
        printf("Payment value must be positive\n");
        return 1;
    }
    acc_t account = get_account(account_number);
    if (account.account_number == NULL_ACCOUNT.account_number){
        printf("Account not found\n");
        return 1;
    }
    if(account.loan_balance == 0){
        printf("No loans to repay\n");
        return 1;
    }
    if (payment_value > account.curr_balance){
        printf("Payment exceeds account balance (%d)\n", account.curr_balance);
        return 1;
    }
    if (payment_value > account.loan_balance){
        printf("Payment too high, exceeds loan balance (%d)\n", account.loan_balance);
        return 1;
    }
    acc_t bank_account = get_account(ROOT_BANK_ACCOUNT.account_number);
    if (bank_account.curr_balance + payment_value > MAX_ACCOUNT_VALUE){
        printf("Bank has too much money, sorry\n");
        return 1;
    }
    account.loan_balance -= payment_value;
    account.curr_balance -= payment_value;
    bank_account.curr_balance += payment_value;
    if (paste_account_at_number(account_number, account)==1 ||
        paste_account_at_number(ROOT_BANK_ACCOUNT.account_number, bank_account)==1)
        return 1;
    else
        return 0;
}

int make_transfer(uint32_t origin_account_number, uint32_t dest_account_number, int32_t transfer_value) {
    acc_t origin_account = get_account(origin_account_number);
    acc_t dest_account = get_account(dest_account_number);
    if (origin_account.account_number == NULL_ACCOUNT.account_number ||
        dest_account.account_number == NULL_ACCOUNT.account_number) {
        printf("Account not found\n");
        return 1;
    }
    if (transfer_value <= 0) {
        printf("Transfer value must be positive\n");
        return 1;
    }
    if (transfer_value > MAX_TRANSFER) {
        printf("Transfer exceeds maximum allowed transfer value\n");
        return 1;
    }
    if (transfer_value > origin_account.curr_balance) {
        printf("Transfer value exceeds account balance\n");
        return 1;
    }
    if (dest_account.curr_balance + transfer_value > MAX_ACCOUNT_VALUE) {
        printf("Transfer exceeds maximum destination account value\n");
        return 1;
    }
    origin_account.curr_balance -= transfer_value;
    dest_account.curr_balance += transfer_value;
    if (paste_account_at_number(origin_account_number, origin_account) == 1 ||
        paste_account_at_number(dest_account_number, dest_account) == 1){
        printf("Transfer failed\n");
        return 1;
    } else {
        printf("Transfer successful\n");
        return 0;
    }
}

int collect_interest(uint32_t account_number){
    acc_t account = get_account(account_number);
    if (account.account_number == NULL_ACCOUNT.account_number){
        printf("Account not found\n");
        return 1;
    }
    if (account.loan_balance == 0){
        return 0;
    }
    int32_t interest_value = account.loan_balance * account.interest_rate;
    if (account.loan_balance + interest_value > MAX_LOAN_VALUE){
        printf("Interest achieved maximum debt in account %ud\n", account_number);
        return 1;
    }
    account.loan_balance += interest_value;
    printf("Interest collected: %d\n", interest_value);
    if (paste_account_at_number(account_number, account)==1)
        return 1;
    else
        return 0;
}

int print_matching_accounts(acc_t pattern_acc, int view_mode){
    FILE *file = fopen(RECORD_FILE, "r");
    if (file == NULL){
        printf("Error opening file\n");
        return 1;
    }
    acc_t retrieved_account;
    bool found_match = false;
    while (fread(&retrieved_account, sizeof(acc_t), 1, file)){
        if ((pattern_acc.name[0] == NULL_ACCOUNT.name[0] || strncmp(pattern_acc.name, retrieved_account.name, strlen(pattern_acc.name) - 1) == 0 ) &&
            (pattern_acc.surname[0] == NULL_ACCOUNT.surname[0] || strncmp(pattern_acc.surname, retrieved_account.surname, strlen(pattern_acc.surname) - 1) == 0 ) &&
            (pattern_acc.address[0] == NULL_ACCOUNT.address[0] || strncmp(pattern_acc.address, retrieved_account.address, strlen(pattern_acc.address) - 1) == 0 ) &&
            (pattern_acc.national_id[0] == NULL_ACCOUNT.national_id[0] || strncmp(pattern_acc.national_id, retrieved_account.national_id, strlen(pattern_acc.national_id) - 1) == 0))
        {
            if(!found_match){
                print_table_header(view_mode);
                found_match = true;
            }
            print_account_as_table(retrieved_account, view_mode);
        }
    }
    if (!found_match)
        printf("No matching accounts found\n");
    fclose(file);
    return 0;
}

int search_for_account(uint32_t search_option, char* prev_search_string){
    acc_t account = NULL_ACCOUNT;
    char search_string[MAX_COMMAND_LENGTH];
    bool no_same_line_arg_passed = true;
    if (strlen(prev_search_string)>1) {
        no_same_line_arg_passed = false;
        while (*prev_search_string==' ' || *prev_search_string=='\t')
            prev_search_string++;
        strncpy(search_string, prev_search_string, MAX_COMMAND_LENGTH);
    }
    switch (search_option) {
        case 0:
            if (no_same_line_arg_passed) {
                printf("enter account number\n");
                get_and_clean_input(search_string, MAX_COMMAND_LENGTH);
            }
            account = get_account(strtol(search_string, NULL, 10));
            if (account.account_number == NULL_ACCOUNT.account_number){
                printf("Account not found\n");
                return 1;
            }
            print_table_header(global_view_mode);
            print_account_as_table(account, global_view_mode);
            return 0;
        case 1:
            if(no_same_line_arg_passed){
                printf("enter name\n");
                get_and_clean_input(account.name, LENGTH_OF_NAME);
            } else {
                strncpy(account.name, search_string, LENGTH_OF_NAME);
            }
            break;
        case 2:
            if(no_same_line_arg_passed){
                printf("enter surname\n");
                get_and_clean_input(account.surname, LENGTH_OF_SURNAME);
            } else {
                strncpy(account.surname, search_string, LENGTH_OF_SURNAME);
            }
            break;
        case 3:
            if(no_same_line_arg_passed){
                printf("enter address\n");
                get_and_clean_input(account.address, LENGTH_OF_ADDRESS);
            } else {
                strncpy(account.address, search_string, LENGTH_OF_ADDRESS);
            }
            break;
        case 4:
            if(no_same_line_arg_passed){
                printf("enter national ID\n");
                get_and_clean_input(account.national_id, LENGTH_OF_NATIONAL_ID);
            } else {
                strncpy(account.national_id, search_string, LENGTH_OF_NATIONAL_ID);
            }
            break;
        default:
            printf("Invalid search option\n");
            print_search_help();
            return 1;
    }
    print_matching_accounts(account, global_view_mode);
    return 0;
}

int check_string_for_command(char* string, const char* command){
    if (strncmp(string, command, strlen(command)) == 0){
        return 1;
    }
    return 0;
}

int read_command() {
    char command[MAX_COMMAND_LENGTH];
    int cmd_id;
    // predeclared memory space for arguments (different types for different values, f.eg. uint32_t for account number, int32_t for money)
    uint32_t arg1;
    int32_t arg2;
    uint32_t arg3;
    char* endptr;
    printf("BankOS:root> ");
    get_and_clean_input(command, MAX_COMMAND_LENGTH);
    for (cmd_id = 0; cmd_id < N_OF_COMMANDS; cmd_id++) {
        if (check_string_for_command(command, COMMANDS[cmd_id]))
            break;
    }
    switch(cmd_id){
        case 0: // list
            arg2 = strtol(command+strlen(COMMANDS[cmd_id]), NULL, 10)+1;
            read_all_records(arg2);
            break;
        case 1: // add
            add_account_from_input();
            break;
        case 2: // deposit
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg2 = strtol(endptr, NULL, 10);
            make_deposit(arg1, arg2);
            break;
        case 3: // withdraw
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg2 = strtol(endptr, NULL, 10);
            make_withdraw(arg1, arg2);
            break;
        case 4: // borrow
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg2 = strtol(endptr, NULL, 10);
            take_loan(arg1, arg2);
            break;
        case 5: // repay
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg2 = strtol(endptr,NULL, 10);
            repay_loan(arg1, arg2);
            break;
        case 6: // transfer
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg3 = strtol(endptr, &endptr, 10);
            arg2 = strtol(endptr, NULL, 10);
            make_transfer(arg1, arg3, arg2);
            break;
        case 7: // search
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            search_for_account(arg1, endptr);
            break;
        case 8: // get
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg2 = strtol(endptr, NULL, 10)+1;
            print_table_header(arg2);
            if (arg1 == 0)
                print_account_as_table(get_last_account(), arg2);
            else
                print_account_as_table(get_account(arg1), arg2);
            break;
        case 9: // quit
            return 1;
        case 10: // reset_file
            reset_file();
            populate_file_with_preset_accounts();
            break;
        case 11: // collect_interest
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), NULL, 10);
            collect_interest(arg1);
            break;
        case 12: // help
            print_help();
            break;
        case 13: // paste
            arg1 = strtol(command+strlen(COMMANDS[cmd_id]), &endptr, 10);
            arg3 = strtol(endptr, &endptr, 10);
            printf("account to be pasted to position %d:\n", arg1);
            print_table_header(global_view_mode);
            print_account_as_table(get_account(arg3), global_view_mode);
            paste_account_at_number(arg1, get_account(arg3));
            break;
        default:
            printf("Command not recognized\n");
            break;
    }
    return 0;
}

int main() {
    //reset_file();
    print_welcome_screen();
    verify_file_integrity();
    acc_t last_account = get_last_account();
    number_of_accounts = last_account.account_number;
    REQUIRE_CONFIRMATION_ON_EDIT = true;
    while(1) {
        if(read_command()==1){
            break;
        }
    }
    return 0;
}
