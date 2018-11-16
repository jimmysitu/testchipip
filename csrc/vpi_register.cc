/*
 * VPI register
 * -----------------------------------------------------------------------
 * Jimmy Situ (web@jimmystone.cn)
 */
#include <vpi_user.h>
#include <sv_vpi_user.h>


/*
 * This is a table of register functions. This table is the external
 * symbol that the simulator looks for when loading this .vpi module.
 */
extern void serial_tick_register();

void (*vlog_startup_routines[])(void) = {
      serial_tick_register,
      0
};
