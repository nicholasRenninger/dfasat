#! /usr/bin/python
# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import sys
import logging
import pygccxml
import pyplusplus

#pyplusplus.module_builder.set_logger_level(logging.DEBUG)
#pygccxml.utils.loggers.set_level(logging.DEBUG)

# types that include apta_node in their definition
apta_node_types = ["apta_node *", "state_set", "merge_map"]

import subprocess
clibpath = subprocess.check_output(['find /usr/lib/gcc -name stddef.h | head -n 1'], shell=True)[:-17].decode('utf-8')
cincpath = subprocess.check_output(['find /usr/include/c++ -name iostream | head -n 1'], shell=True)[:-9].decode('utf-8')

generator_path, generator_name = pygccxml.utils.find_xml_generator('castxml')
xml_generator_config = pygccxml.parser.xml_generator_configuration_t(
    xml_generator=generator_name,
    xml_generator_path=generator_path,
    cflags="-std=c++11",
    include_paths=[".", "lib", "evaluation", os.path.join(clibpath, 'include'), cincpath],# "/usr/include/c++/5"],
)


mb = pyplusplus.module_builder.module_builder_t(
        files=['main.cpp'],
        xml_generator_config=xml_generator_config,
        indexing_suite_version=2
)


print("done builder")

print("setting call policies")


apta = mb.class_( 'apta' )
apta.include()
for i in apta.member_functions():
  for type_ in apta_node_types:
    if type_ in i.return_type.decl_string:
      i.call_policies = pyplusplus.module_builder.call_policies.return_internal_reference(1)

apta_node = mb.class_( 'apta_node' )
apta_node.include()
a_n_ret = mb.member_functions( return_type='::apta_node *' )
a_n_ret.call_policies = pyplusplus.module_builder.call_policies.return_internal_reference(1)

state_merger = mb.class_( 'state_merger' )
state_merger.include()
for i in state_merger.member_functions():
  for type_ in apta_node_types:
    if type_ in i.return_type.decl_string:
      i.call_policies = pyplusplus.module_builder.call_policies.return_internal_reference(1)


dfasat = mb.free_function( 'dfasat' )
dfasat.include()

print('finding evaluation_functions')

def find_derived(class_):
  derived = []
  for c in class_.derived:
    derived += find_derived(c.related_class)
  return [class_] + derived

for c in find_derived(mb.class_(name="evaluation_function")):
  print("Including "+c.name)
  c.include()


#for decl in mb.decls():
#  if "DerivedDataRegister" in decl.name:
#    print("Including "+decl.name)
#    decl.include()

merge_map = mb.global_ns.typedefs("merge_map")
merge_map.rename('merge_map')
merge_map.include()

#import code
#code.interact(local=locals())

#import code
#code.interact(local=locals())

#print('printing declarations of apta_node')
#mb.print_declarations(apta_node)


"""#rename enum Color to color
Color = mb.enum( 'color' )
Color.rename('Color')

#Set call policies to animal::genealogical_tree_ref
animal = mb.class_( 'animal' )
genealogical_tree_ref = animal.member_function( 'genealogical_tree_ref', recursive=False )
genealogical_tree_ref.call_policies = module_builder.call_policies.return_internal_reference()

#next code has same effect
genealogical_tree_ref = mb.member_function( 'genealogical_tree_ref' )
genealogical_tree_ref.call_policies = module_builder.call_policies.return_internal_reference()

#I want to exclude all classes with name starts with impl
impl_classes = mb.classes( lambda decl: decl.name.startswith( 'impl' ) )
impl_classes.exclude()

#I want to exclude all functions that returns pointer to int
ptr_to_int = mb.free_functions( return_type='int *' )
ptr_to_int.exclude()

#I can print declarations to see what is going on
mb.print_declarations()

#I can print single declaration
#mb.print_declarations( animal )
mb.print_declarations( Color )
"""
#Now it is the time to give a name to our module
mb.build_code_creator( module_name='dfasat' )

#mb.add_registration_code("""boost::python::converter::registry::insert
#    (convert_to_FILEptr,
#         boost::python::type_id<FILE>(),
#	      &boost::python::converter::wrap_pytype<&PyFile_Type>::get_pytype);""")

#It is common requirement in software world - each file should have license
mb.code_creator.license = '// license'

#I don't want absolute includes within code
mb.code_creator.user_defined_directories.append( os.path.abspath('.') )

#And finally we can write code to the disk
mb.write_module( os.path.join( os.path.abspath('.'), 'generated.cpp' ) )
