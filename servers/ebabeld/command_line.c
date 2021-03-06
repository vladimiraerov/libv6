/**
 * command_line.c
 *
 * Dave Taht
 * 2017-03-13
 */

// I have been writing command line parsers this way for a long time.
// I don't know why.

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <iconv.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// In order to get registers may have to declare these
// even earlier than this

#include "shared.h"
#include "version.h"
#include "command_line.h"

typedef struct {
  const int id;
  const char* const desc;
} output_types_t;

enum outputformats {
  FORMATTERM,
  FORMATSYSLOG,
  FORMATJSON,
  FORMATTEXT,
  FORMATCSV,
  FORMATRAW,
  FORMATMAX
};

static const output_types_t output_type[] = {
  { FORMATSYSLOG, "syslog" },
  { FORMATJSON, "json" },
  { FORMATTERM, "term" },
  { FORMATTEXT, "text" },
  { FORMATCSV, "csv" },
  { FORMATRAW, "raw" },
  { 0, NULL },
};

#define FORMATDEFAULT FORMATTEXT

int usage(char* err)
{
  if(err) fprintf(stderr, "%s\n", err);
  printf("ebabeld %s [options]\n", EBABELD_VERSION);
  printf("-V --version                  print the version and exit\n"
         "-m --multicast_address  \n"
         "-p --port number    \n"
         "-S --state_file  \n"
         "-h --hello_interval      \n"
         "-H --wired_hello_interval  \n"
         "-z --diversity    \n"
         "-M --half_time     \n"
         "-k --kernel_priority    \n"
         "-A --duplicate_priority    \n"
         "-l --iff_running    \n"
         "-w --deoptimize_wired    \n"
         "-s --split_horizon     \n"
         "-r --random_id       \n"
         "-u --flush_invisible   \n"
         "-d --debug  \n"
         "-g --config_port  \n"
         "-t --table\n"
         "-T --table_export\n"
         "-c --config_file\n"
         "-D --daemonize\n"
         "-L --logfile\n"
         "-C --config_statement\n"
         "-I --pidfile\n"

#ifdef HAVE_EXTENSIONS
         "-? --help         this message\n"
         "   --about         [credits]\n"
         "-U --use_unicast \n"
         "-4 --ipv4 listen on ipv4 only\n"
         "-5 --stubby\n"
         "-6 --ipv6 listen on ipv6 only\n"
         "-_ --monitor   monitor routing traffic\n"
         "-+ --secure    use secure transports\n"
         "-= --trust     trust other routers\n"
         "-@ --traps trap on these errors\n"
         "-^ --udp_lite use udp-lite exclusively\n"
         "   --plugin\n"
#endif

         );

  exit(-1);
}

static const struct option long_options[] =
{ { "version", no_argument, NULL, 'v' },
  { "multicast_address", required_argument, NULL, 'm' },
  { "port", required_argument, NULL, 'p' },
  { "state_file", required_argument, NULL, 'S' },
  { "hello_interval", required_argument, NULL, 'h' },
  { "wired_hello_interval", required_argument, NULL, 'H' },
  { "diversity", required_argument, NULL, 'z' },
  { "half_time", required_argument, NULL, 'M' },
  { "kernel_priority", required_argument, NULL, 'k' },
  { "duplicate_priority", required_argument, NULL, 'A' },
  { "iff_running", no_argument, NULL, 'l' },
  { "deoptimize_wired", no_argument, NULL, 'w' },
  { "split_horizon", required_argument, NULL, 's' },
  { "random_id", no_argument, NULL, 'r' },
  { "flush_invisible", no_argument, NULL, 'u' },
  { "debug", required_argument, NULL, 'd' },
  { "config_port", required_argument, NULL, 'g' },
  { "table", required_argument, NULL, 't' },
  { "table_export", required_argument, NULL, 'T' },
  { "config_file", required_argument, NULL, 'c' },

  { "daemonize", no_argument, NULL, 'D' },
  { "logfile", required_argument, NULL, 'L' },
  { "config_statement", required_argument, NULL, 'C' },
  { "pidfile", required_argument, NULL, 'I' },
  { "help", no_argument, NULL, '?' },

#ifdef HAVE_EXTENSIONS
  { "about", no_argument, NULL, '&' },
  { "output_format", required_argument, NULL, '0' },
  { "use_unicast", no_argument, NULL, 'U' },
  { "ipv4", no_argument, NULL, '4' },
  { "stubby", no_argument, NULL, '5' },
  { "ipv6", no_argument, NULL, '6' },
  { "secure", no_argument, NULL, '+' },
  { "trust", required_argument, NULL, '=' },
  { "monitor", required_argument, NULL, '_' },
  { "plugin", required_argument, NULL, 'G' },
  { "traps", required_argument, NULL, '@' },
  { "dry_run", no_argument, NULL, '1' },
  { "udp_lite", no_argument, NULL, '^' },
#endif
  { NULL, 0, NULL, 0 } };

#define penabled(a)                                                          \
  if(o->a) fprintf(fp, "" #a " ");
#define BOOLOPT(OPTION) OPTION = (strtoul(optarg, NULL, 10) & 1)

#ifdef HAVE_EXTENSIONS
#define QSTRING "Vm:p:S:h:H:z:M:k:A:lwsrud:g:t:T:c:DL:C:I:?U456ST:M:G:^"
#else
#define QSTRING "Vm:p:S:h:H:z:M:k:A:lwsrud:g:t:T:c:DL:C:I:"
#endif

static char* plugin_load(char* a) { return NULL; }

static int getl(char* arg) { return strtoul(arg, NULL, 10); }

/* Yes, I am passing this structure around. Try to imagine
   we're dealing with hardware here. */

CommandLineOpts_t process_options(int argc, char** argv, CommandLineOpts_t o)
{
  int option_index = 0;
  int opt = 0;

  optind = 1;

  while(true) {
    opt = getopt_long(argc, argv, QSTRING, long_options, &option_index);
    if(opt == -1) break;

    switch(opt) {
    case 'V':
      o.version = 1;
      break;
    case 'm':
      o.mcast_address = optarg;
      break;
    case 'p':
      o.port = getl(optarg);
      break;
    case 'S':
      o.state_file = optarg;
      break;
    case 'z':
      o.g.diversity = getl(optarg);
      break;
    case 'h':
      o.hello_interval = getl(optarg);
      break;
    case 'H':
      o.wired_hello_interval = getl(optarg);
      break;
    case 'M':
      o.half_time = getl(optarg);
      break;
    case 'k':
      o.kernel_priority = getl(optarg);
      break;
    case 'A':
      o.duplicate_priority = getl(optarg);
      break;
    case 'l':
      o.g.iff_running = 1;
      break;
    case 'w':
      o.g.deoptimize_wired = 1;
      break;
    case 's':
      o.g.split_horizon = 1;
      break;
    case 'r':
      o.g.random_id = 1;
      break;
    case 'u':
      o.g.flush_invisible = 1;
      break;
    case 'd':
      o.g.debug = getl(optarg);
      break;
    case 'g':
      o.config_port = getl(optarg);
      break;
    case 't':
      o.table = getl(optarg);
      break; // FIXME multiple tables
    case 'T':
      o.table_export = getl(optarg);
      break;
    case 'c':
      o.config_file = optarg;
      break;
    case 'I':
      o.pid_file = optarg;
      break;
    case '?':
      usage("ebabeld: a high speed babel protocol routing daemon");
      break;

#ifdef HAVE_EXTENSIONS
    case '&':
      o.about = 1;
      break;
    case '0':
      o.output_format = 1;
      break; // FIXME
    case 'U':
      o.g.use_unicast = 1;
      break;
    case '4':
      o.g.ipv6 = 0;
      break;
    case '5':
      o.g.stubby = 1;
      break;
    case '6':
      o.g.ipv4 = 0;
      break;
    case '_':
      o.g.monitor = 1;
      break;
    case '+':
      o.secure = 1;
      break; // unimplemented
    case '=':
      o.trust = 1;
      break; // unimplemented
    case '@':
      o.traps = getl(optarg);
      break;
    case '^':
      o.udplite = 1;
      break;
    case '1':
      o.dry_run = 1;
      break;
    case 'G':
      o.plugins = 1;
      char* plugin = plugin_load(optarg);
      if(plugin == NULL) fprintf(stderr, "plugin %s not supported\n", optarg);
      break;
#endif
    default:
      fprintf(stderr, "%d", opt);
      usage("Invalid option");
      break;
    }
  }

  return o;
}

CommandLineOpts_t ebabeld_default_options(CommandLineOpts_t o)
{
  o.g.ipv4 = 1;
  o.g.ipv6 = 1;
  o.format = FORMATDEFAULT; // term
  o.mcast_address = default_protocol_v6addr;
  o.state_file = default_protocol_state_file;
  o.config_file = default_protocol_config_file;
  o.pid_file = default_protocol_pid_file;
  o.transport1 = default_protocol_v6transport1;
  o.transport2 = default_protocol_v6transport2;
  o.id_file = default_protocol_id_file;
  o.port = default_protocol_v6port;
  o.config_port = default_protocol_config_port;
  o.config_local = default_config_local_mux;
  o.hello_interval = default_protocol_hello_interval;
  o.wired_hello_interval = default_protocol_wired_hello_interval;
  o.half_time = default_protocol_half_time;
  o.kernel_priority = default_protocol_kernel_priority;
  o.duplicate_priority = default_protocol_duplicate_priority;
  o.v4kernel_metric = default_base_v4_metric;
  o.v6kernel_metric = default_base_v6_metric;
  o.config_address = default_config_address;
  o.traps = default_protocol_traps;
  o.allowed_gid = default_config_allowed_gid;
  o.keys_file = default_protocol_keys_file;
  o.config_keys_file = default_protocol_config_keys_file;
  o.my_keys_file = default_protocol_config_mykeys_file;
  o.my_pub_file = default_protocol_config_mypub_file;

#ifndef PRODUCTION
  o.udplite = true;
#endif
  return o;
  // ebabeld_default_language(o);
}

#ifdef DEBUG_MODULE

global_flags_t tflags;
int main(int argc, char** argv)
{
  CommandLineOpts_t o = { 0 };
  o = process_options(argc, argv, ebabeld_default_options(o));
  tflags = o.g;
  return (0);
}
#endif
