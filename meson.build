project('obmc-webserver', 'cpp',
        version : '1.0',
        meson_version: '>=0.53.2',
        default_options: [
            'werror=true',
            'warning_level=3',
            'cpp_std=c++2a',
            'buildtype=debugoptimized',
            'b_ndebug=if-release',
            'b_lto=true',
# enable when https://github.com/chriskohlhoff/asio/issues/533
# is resolved. ASIO default executor doesn't build with no-rtti
           #'cpp_rtti=false'
           ])

# Project related links

project_pretty_name = 'obmc-webserver'
project_url = 'https://github.com/openbmc/' + project_pretty_name
project_issues_url = project_url + '/issues/new'
summary('Issues',project_issues_url, section: 'Report Issues')

# Validate the c++ Standard

if get_option('cpp_std') != 'c++2a'
    error('This project requires c++2a support')
endif

# Get compiler and default build type

cxx = meson.get_compiler('cpp')
build = get_option('buildtype')
optimization = get_option('optimization')
summary('Build Type',build, section : 'Build Info')
summary('Optimization',optimization, section : 'Build Info')

# remove debug information for minsize buildtype
if(get_option('buildtype') == 'minsize')
  add_project_arguments('-DNDEBUG', language : 'cpp')
endif

# Disable lto when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-fno-lto', language: 'cpp')
  message('Disabling lto & its supported features as optimization is disabled')
endif

# Include Directories

incdir = include_directories('include')

# Get the options and enable the respective features
## create a MAP of  "options : feature_flag"

feature_map = {
}

logging_levels = {
  'emerg': '-DBMC_LOGGING_EMERGENCY',
  'alert': '-DBMC_LOGGING_FATAL',
  'critical': '-DBMC_LOGGING_CRITICAL',
  'error': '-DBMC_LOGGING_ERROR',
  'warning': '-DBMC_LOGGING_WARNING',
  'notice': '-DBMC_LOGGING_NOTICE',
  'info': '-DBMC_LOGGING_INFO',
  'debug': '-DBMC_LOGGING_DEBUG',
}
# Get the options status and build a project summary to show which flags are
# being enabled during the configuration time.

foreach option_key,option_value : feature_map
  if(get_option(option_key).enabled())
    if(option_key == 'mutual-tls-auth' or option_key == 'insecure-disable-ssl')
      if(get_option('insecure-disable-ssl').disabled() or get_option('mutual-tls-auth').disabled())
        add_project_arguments(option_value,language:'cpp')
        summary(option_key,option_value, section : 'Enabled Features')
      endif
    else
      summary(option_key,option_value, section : 'Enabled Features')
      add_project_arguments(option_value,language:'cpp')
    endif
  else
      if(option_key == 'insecure-disable-ssl')
        summary('ssl','-DBMC_ENABLE_SSL', section : 'Enabled Features')
        add_project_arguments('-DBMC_ENABLE_SSL', language : 'cpp')
      endif
  endif
endforeach

if(get_option('tests').enabled())
  summary('unittest','NA', section : 'Enabled Features')
endif

if get_option('yocto-deps').disabled()
  # This is an out of tree build, so enable ibm console flag
  # for CI purpose.
  add_project_arguments('-DBMC_ENABLE_IBM_MANAGEMENT_CONSOLE', language: 'cpp')
  summary('ibm-management-console','-DBMC_ENABLE_IBM_MANAGEMENT_CONSOLE',section : 'Enabled Features')
endif

# Add compiler arguments

# -Wpedantic, -Wextra comes by default with warning level
add_project_arguments(
  cxx.get_supported_arguments([
  '-Wold-style-cast',
  '-Wcast-align',
  '-Wunused',
  '-Woverloaded-virtual',
  '-Wconversion',
  '-Wsign-conversion',
  '-Wno-attributes',
  ]),
  language: 'cpp'
)

if (cxx.get_id() == 'clang' and cxx.version().version_compare('>9.0'))
add_project_arguments(
  cxx.get_supported_arguments([
    '-Weverything',
    '-Wno-c++98-compat',
    '-Wno-c++98-compat-pedantic',
    '-Wno-global-constructors',
    '-Wno-exit-time-destructors',
    '-Wno-shadow',
    '-Wno-used-but-marked-unused',
    '-Wno-documentation-unknown-command',
    '-Wno-weak-vtables',
    '-Wno-documentation',
    '-Wno-padded',
    '-Wunused-parameter',
    '-Wcovered-switch-default',
    '-Wcomma',
    '-Wextra-semi',
    '-Wzero-as-null-pointer-constant',
    '-Wswitch-enum',
    '-Wnull-dereference',
    '-Wdouble-promotion',
    '-Wformat=2',
  ]),
  language:'cpp')
endif

# if compiler is gnu-gcc , and version is > 8.0 then we add few more
# compiler arguments , we know that will pass

if (cxx.get_id() == 'gcc' and cxx.version().version_compare('>8.0'))

  ## remove once bmcweb/issues/147 is fixed
  add_global_link_arguments('-Wno-stringop-overflow',language:['c','cpp'])
  add_project_arguments('-Wno-stringop-overflow',language:['c','cpp'])

  add_project_arguments(
    cxx.get_supported_arguments([
     '-Wduplicated-cond',
     '-Wduplicated-branches',
     '-Wlogical-op',
     '-Wunused-parameter',
     '-Wnull-dereference',
     '-Wdouble-promotion',
     '-Wformat=2',
     ]),
    language:'cpp')

  if (get_option('buildtype') != 'plain')
    if (get_option('b_lto') == true and get_option('optimization')!='0')
      # Reduce the binary size by removing unnecessary
      # dynamic symbol table entries

      add_project_arguments(
       cxx.get_supported_arguments([
       '-fno-fat-lto-objects',
       '-fvisibility=hidden',
       '-fvisibility-inlines-hidden'
       ]),
       language: 'cpp')

      if cxx.has_link_argument('-Wl,--exclude-libs,ALL')
        add_project_link_arguments('-Wl,--exclude-libs,ALL', language: 'cpp')
      endif
  endif

  add_project_arguments(
    [logging_levels[get_option('bmc-logging')]],
    language : 'cpp'
  )

  summary(
    {
      'log-level' : get_option('bmc-logging')
    },
    section : 'Enabled Features'
  )
endif

# Set Compiler Security flags

security_flags = [
'-fstack-protector-strong',
'-fPIE',
'-fPIC',
'-D_FORTIFY_SOURCE=2',
'-Wformat',
'-Wformat-security'
]

## Add security flags for builds of type 'release','debugoptimized' and 'minsize'

if not (get_option('buildtype') == 'plain' or get_option('buildtype').startswith('debug'))
  add_project_arguments(
   cxx.get_supported_arguments([
    security_flags
  ]),
  language: 'cpp')
endif


# Find the dependency modules, if not found use meson wrap to get them
# automatically during the configure step
obmc_webserver_dependencies = []

pam = cxx.find_library('pam', required: get_option('pam'))
atomic =  cxx.find_library('atomic', required: true)
openssl = dependency('openssl', required : true)
obmc_webserver_dependencies += [pam, atomic, openssl]

sdbusplus = dependency('sdbusplus',required : false)
if not sdbusplus.found()
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbusplus = sdbusplus.as_system('system')
endif
obmc_webserver_dependencies += sdbusplus

systemd = dependency('systemd')
zlib = dependency('zlib')
obmc_webserver_dependencies += [systemd, zlib]

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    subproject('nlohmann', required: false)
    nlohmann_json = declare_dependency(
        include_directories: [
            'subprojects/nlohmann/single_include',
            'subprojects/nlohmann/single_include/nlohmann',
        ]
    )
endif
obmc_webserver_dependencies += nlohmann_json

# Source files

srcfiles_obmc_webserver = ['src/main.cpp']

srcfiles_unittest = []

# Gather the Configuration data

conf_data = configuration_data()
conf_data.set('BMC_HTTP_REQ_BODY_LIMIT_MB',get_option('http-body-limit'))
conf_data.set('BMC_CONNECTION_COUNT_LIMIT',get_option('connection-count-limit'))
conf_data.set('MESON_INSTALL_PREFIX',get_option('prefix'))
configure_file(output: 'config.h',
               configuration: conf_data)

# Configure and install systemd unit files

systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

bindir = get_option('prefix') + '/' +get_option('bindir')

summary({
          'prefix' : get_option('prefix'),
          'bindir' : bindir,
          'systemd unit directory' : systemd_system_unit_dir
        }, section : 'Directories')

configure_file(input : 'obmc-webserver.socket',
               output : 'obmc-webserver.socket',
               copy : true,
               install_dir: systemd_system_unit_dir,
               install : true)

configure_file(input : 'obmc-webserver.service.in',
               output : 'obmc-webserver.service',
               install_dir: systemd_system_unit_dir,
               configuration: conf_data,
               install : true)

# Copy pam-webserver to etc/pam.d
configure_file(input : 'pam-webserver',
               output : 'webserver',
               copy : true,
               install_dir: '/etc/pam.d',
               install : true)

install_subdir('static', install_dir : 'share/www', strip_directory : true)

# Generate the obmc-webserver executable and the test binary

executable('obmc-webserver', srcfiles_obmc_webserver,
            include_directories : incdir,
            dependencies: obmc_webserver_dependencies,
            install: true,
            install_dir:bindir)

if(get_option('tests').enabled())
  foreach src_test : srcfiles_unittest
    testname = src_test.split('/')[-1].split('.')[0]
    test(testname,executable(testname,src_test,
                include_directories : incdir,
                install_dir: bindir,
                dependencies: [ gtest,openssl,gmock,nlohmann_json,sdbusplus ]))
  endforeach
endif
