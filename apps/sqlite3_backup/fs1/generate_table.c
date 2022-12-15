extern void sqlite3_generate_table();

__attribute__((section(".text"))) int main()
{
    sqlite3_generate_table();
    return 0;
}
