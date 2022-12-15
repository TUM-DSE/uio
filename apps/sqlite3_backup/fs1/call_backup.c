extern void sqlite3_save();

__attribute__((section(".text"))) int main()
{
    sqlite3_save();
    return 0;
}
