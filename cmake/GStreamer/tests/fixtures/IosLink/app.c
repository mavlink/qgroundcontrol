void gst_init_static_plugins(void);
void qgc_dump_pipeline(void);

int main(void)
{
    gst_init_static_plugins();
    qgc_dump_pipeline();
    return 0;
}
