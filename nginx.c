
int
main(int argc, char *argv[])
{
    ngx_init_signals();
    ngx_master_process_cycle();

    return 0;
} 
