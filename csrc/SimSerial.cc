#include <vpi_user.h>
#ifdef __ICARUS__
  #include <sv_vpi_user.h>
#else
  #include <svdpi.h>
#endif
#include <vector>
#include <string>
#include <fesvr/tsi.h>

tsi_t *tsi = NULL;

static inline int copy_argv(int argc, char **argv, char **new_argv)
{
    int optind = 1;
    int new_argc = argc;

    new_argv[0] = argv[0];

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '+') {
            optind = i - 1;
            new_argc = argc - i + 1;
            break;
        }
    }

    for (int i = 1; i < new_argc; i++)
        new_argv[i] = argv[i + optind];

    return new_argc;
}

#ifdef __ICARUS__
static PLI_INT32 serial_tick_calltf(PLI_BYTE8 *nouse)
#else
extern "C" int serial_tick(
        unsigned char out_valid,
        unsigned char *out_ready,
        int out_bits,

        unsigned char *in_valid,
        unsigned char in_ready,
        int *in_bits)
#endif
{
#ifdef __ICARUS__
    unsigned char out_valid;    //0
    unsigned char out_ready;    //1, out
    int out_bits;               //2

    unsigned char in_valid;     //3, out
    unsigned char in_ready;     //4
    int in_bits;                //5, out
    vpiHandle sys = vpi_handle(vpiSysTfCall, 0);
    vpiHandle argv = vpi_iterate(vpiArgument, sys);
    vpiHandle arg[6];
    s_vpi_value val[6];
    s_vpi_value rtn;

    // link all signals
    for(int i = 0; i < 6; i++) {
        arg[i] = vpi_scan(argv);
        val[i].format = vpiIntVal;
    }
    vpi_free_object(argv);
#endif

#ifdef __ICARUS__
    for(int i = 0; i < 6; i++) {
        vpi_get_value(arg[i], &(val[i]));
    }
    out_valid = (unsigned char)val[0].value.integer;
    out_ready = (unsigned char)val[1].value.integer;
    out_bits  = val[2].value.integer;

    in_valid = (unsigned char)val[3].value.integer;
    in_ready = (unsigned char)val[4].value.integer;
    in_bits  = val[5].value.integer;

    bool out_fire = out_ready && out_valid;
    bool in_fire = in_valid && in_ready;
    bool in_free = !(in_valid);
#else
    bool out_fire = *out_ready && out_valid;
    bool in_fire = *in_valid && in_ready;
    bool in_free = !(*in_valid);
#endif

    if (!tsi) {
        s_vpi_vlog_info info;
        if (!vpi_get_vlog_info(&info))
          abort();

        char **argv = (char **) malloc(sizeof(char*) * info.argc);
        int argc = copy_argv(info.argc, info.argv, argv);

        tsi = new tsi_t(argc, argv);
    }

    tsi->tick(out_valid, out_bits, in_ready);
    tsi->switch_to_host();

#ifdef __ICARUS__
    in_valid = tsi->in_valid();
    in_bits = tsi->in_bits();
    out_ready = tsi->out_ready();
    val[3].value.integer = in_valid;
    val[5].value.integer = in_bits;
    val[1].value.integer = out_ready;

    vpi_put_value(arg[3], &(val[3]), 0, vpiNoDelay);
    vpi_put_value(arg[5], &(val[5]), 0, vpiNoDelay);
    vpi_put_value(arg[1], &(val[1]), 0, vpiNoDelay);

    // return value
    rtn.format = vpiIntVal;
    rtn.value.integer = tsi->done() ? (tsi->exit_code() << 1 | 1) : 0;
    vpi_put_value(sys, &rtn, 0, vpiNoDelay);
    return 0;
#else
    *in_valid = tsi->in_valid();
    *in_bits = tsi->in_bits();
    *out_ready = tsi->out_ready();

    return tsi->done() ? (tsi->exit_code() << 1 | 1) : 0;
#endif
}

#ifdef __ICARUS__
void serial_tick_register(void)
{
      s_vpi_systf_data tf_data;

      tf_data.type        = vpiSysFunc;
      tf_data.sysfunctype = vpiIntFunc;
      tf_data.tfname    = "$serial_tick";
      tf_data.calltf    = serial_tick_calltf;
      tf_data.sizetf    = 0;
      tf_data.compiletf = 0;
      vpi_register_systf(&tf_data);
}
#endif
