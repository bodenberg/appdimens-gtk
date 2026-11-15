project('appdimens-gtk', 'c', version: '3.2.0', license: 'Apache-2.0',
        meson_version: '>= 0.59', default_options: ['c_std=c99', 'warning_level=2'])

gtk_major = get_option('gtk')
if gtk_major == '3'
  gtk_dep = dependency('gtk+-3.0', version: '>= 3.22')
  lib_name = 'appdimens-gtk3'
  gir_gtk = 'Gtk-3.0'
else
  gtk_dep = dependency('gtk4', version: '>= 4.0')
  lib_name = 'appdimens-gtk4'
  gir_gtk = 'Gtk-4.0'
endif
m_dep = meson.get_compiler('c').find_library('m', required: false)
inc = include_directories('include')

core_dep = declare_dependency(include_directories: inc)
lib = library(lib_name, 'src/appdimens-gtk.c',
  include_directories: inc, dependencies: [gtk_dep, m_dep], version: meson.project_version(),
  c_args: ['-Wno-unused-parameter'], install: true)
appdimens_gtk_dep = declare_dependency(link_with: lib, include_directories: inc, dependencies: [gtk_dep, m_dep])
meson.override_dependency(lib_name, appdimens_gtk_dep)

install_headers('include/appdimens/appdimens.h', 'include/appdimens/appdimens_custom.h', 'include/appdimens/appdimens-gtk.h', subdir: 'appdimens')
import('pkgconfig').generate(lib, name: lib_name, description: 'AppDimens for GTK — responsive sdp/ssp sizes',
  requires: gtk_dep, subdirs: ['appdimens', '.'])

gir_dep = dependency('gobject-introspection-1.0', required: get_option('introspection'))
if gir_dep.found()
  gnome = import('gnome')
  gnome.generate_gir(lib,
    sources: ['include/appdimens/appdimens-gtk.h', 'src/appdimens-gtk.c'],
    namespace: 'AppDimens', nsversion: '3', identifier_prefix: 'AppDimens', symbol_prefix: 'appdimens',
    includes: [gir_gtk], header: 'appdimens/appdimens-gtk.h', install: true,
    extra_args: ['--warn-all', '-I' + meson.current_source_dir() / 'include'])
endif

if get_option('tests')
  foreach v : [['core', []], ['core_double', ['-DAD_USE_DOUBLE']], ['core_nolibm', ['-DAD_NO_LIBM']], ['core_fixed', ['-DAD_FIXED_POINT']]]
    test(v[0], executable('test_' + v[0], 'tests/test_core.c', include_directories: [inc, include_directories('tests')],
         c_args: v[1], dependencies: m_dep))
  endforeach
  test('custom', executable('test_custom', 'tests/test_custom.c', include_directories: [include_directories('include/appdimens', 'tests')],
       dependencies: m_dep))
  test('gtk', executable('test_gtk', 'tests/test_gtk.c', dependencies: appdimens_gtk_dep),
       env: ['NO_AT_BRIDGE=1', 'GTK_A11Y=none', 'LSAN_OPTIONS=suppressions=' + meson.current_source_dir() / 'tests' / 'lsan.supp'])
endif
if get_option('examples')
  executable('appdimens-gtk-demo', 'examples/demo.c', dependencies: appdimens_gtk_dep)
endif
