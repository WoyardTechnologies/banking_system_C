#include <stdio.h>

typedef struct Account{
    int account_number;
    char name[32];
    char surname[32];
    char address[128];
    int national_id_number; // PESEL
    int curr_balance;
    int loan_balance;
    float interest_rate;
} account;


int main() {
    printf("Hello, World!\n");
    return 0;
}
