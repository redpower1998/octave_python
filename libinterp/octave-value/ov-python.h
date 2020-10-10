///////////////////////////////////////////////////////////////////////
//
//  This file is used to add Python functions to Matlab, which is 
//  imitated from the Java version. The interface definition is similar.
//  This file will not be officially released by Octave. 
//  So you won’t see it in the official release version. 
//  If you like this feature, please let me know. Then I have the
//  motivation to continue to improve this function.
//  I am currently developing this feature based on personal interests 
//  and hobbies, so it has not been fully implemented. You can continue 
//  to improve it. I don't know why Octave provides official Java support, 
//  but not similar Python support. If Octave needs this feature, I am 
//  willing to continue to improve and contribute to Octave.
//
//  Author:redpower1998@gmail.com
//
////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007-2020 The Octave Project Developers
//
// See the file COPYRIGHT.md in the top-level directory of this
// distribution or <https://octave.org/copyright/>.
//
// This file is part of Octave.
//
// Octave is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING.  If not, see
// <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////

#if !defined(octave_ov_python_h)
#define octave_ov_python_h 1

#include "octave-config.h"

#include "ov.h"
#include "ovl.h"

namespace octave
{
  class type_info;
}

typedef void *voidptr;

class OCTINTERP_API octave_python : public octave_base_value
{
public:
  octave_python(void);

  octave_python(const voidptr &obj);

  octave_python(const octave_python &pobj)
      : octave_base_value(pobj), python_object(nullptr)
  {
    init(pobj.python_object);
  }

  ~octave_python(void) { release(); }

  void *to_python(void) const { return python_object; }

  std::string python_class_name(void) const { return python_classname; }

  octave_base_value *clone(void) const { return new octave_python(*this); }
  octave_base_value *empty_clone(void) const { return new octave_python(); }

  bool is_instance_of(const std::string &) const;

  bool is_defined(void) const { return true; }

  bool is_constant(void) const { return true; }

  bool isstruct(void) const { return false; }

  bool ispython(void) const { return true; }

  string_vector map_keys(void) const;

  dim_vector dims(void) const;

  void print(std::ostream &os, bool pr_as_read_syntax = false);

  void print_raw(std::ostream &os, bool pr_as_read_syntax = false) const;

  bool save_ascii(std::ostream &os);

  bool load_ascii(std::istream &is);

  bool save_binary(std::ostream &os, bool save_as_floats);

  bool load_binary(std::istream &is, bool swap,
                   octave::mach_info::float_format fmt);

  bool save_hdf5(octave_hdf5_id loc_id, const char *name,
                 bool save_as_floats);

  bool load_hdf5(octave_hdf5_id loc_id, const char *name);

  // We don't need to override all three forms of subsref.  The using
  // declaration will avoid warnings about partially-overloaded virtual
  // functions.
  using octave_base_value::subsref;

  octave_value_list
  subsref(const std::string &type,
          const std::list<octave_value_list> &idx, int nargout);

  octave_value
  subsref(const std::string &type, const std::list<octave_value_list> &idx)
  {
    octave_value_list retval = subsref(type, idx, 1);
    return (retval.length() > 0 ? retval(0) : octave_value());
  }

  octave_value subsasgn(const std::string &type,
                        const std::list<octave_value_list> &idx,
                        const octave_value &rhs);

  octave_value
  do_pythonMethod(const std::string &name, const octave_value_list &args);

  static octave_value
  do_pythonObject(const octave_python *module,
                  const std::string &name,
                  const octave_value_list &args);

  octave_value do_python_get(const std::string &name);

  octave_value do_python_set(const std::string &name, const octave_value &val);

private:
  void init(void *pobj);

  void release(void);

private:
  void *python_object;

  std::string python_classname;

public:
  int type_id(void) const { return t_id; }
  std::string type_name(void) const { return t_name; }
  std::string class_name(void) const { return python_classname; }

  static int static_type_id(void) { return t_id; }
  static std::string static_type_name(void) { return t_name; }
  static std::string static_class_name(void) { return "<unknown>"; }
  static void register_type(octave::type_info &);

private:
  static int t_id;
  static const std::string t_name;
};

extern OCTINTERP_API bool Vpython_matrix_autoconversion;

extern OCTINTERP_API bool Vpython_unsigned_autoconversion;

extern OCTINTERP_API bool Vdebug_python;

#endif
