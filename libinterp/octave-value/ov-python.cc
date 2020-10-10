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

//! @file ov-python.cc
//!
//! Provides Octave's Python interface.

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include <clocale>

#include "Cell.h"
#include "builtin-defun-decls.h"
#include "cmd-edit.h"
#include "defaults.h"
#include "defun.h"
#include "error.h"
#include "errwarn.h"
#include "file-ops.h"
#include "file-stat.h"
#include "fpucw-wrappers.h"
#include "interpreter.h"
#include "interpreter-private.h"
#include "load-path.h"
#include "lo-sysdep.h"
#include "oct-env.h"
#include "oct-shlib.h"
#include "ov-python.h"
#include "parse.h"
#include "variables.h"

#if defined(HAVE_PYTHON)
#define PY_SSIZE_T_CLEAN
#include <Python.h>

static bool is_octave_python_init = false;

void octave_python_init()
{
  if (!is_octave_python_init)
  {
    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
  }
}
void octave_python_free()
{
  if (is_octave_python_init)
  {
    Py_Finalize();
  }
}

#endif

#define TO_PYTHON(obj) dynamic_cast<octave_python *>((obj).internal_rep())
#define TO_PYTHON_OBJECT(obj) reinterpret_cast<PyObject *>(obj)

static octave_value
check_exception()
{
  octave_value retval;

  PyObject *err = PyErr_Occurred();
  if (err != NULL)
  {
    PyErr_Print();
    error("[Python] Failed to check exception");
  }
  else
    retval = Matrix();

  return retval;
}
bool Python_IsMatrix(PyObject *pobj, Py_ssize_t &cols, Py_ssize_t &rows)
{
  rows = PyTuple_Size(pobj);
  bool is_matrix = true;
  int sub_item_count = -1;

  for (Py_ssize_t i = 0; i < rows; i++)
  {
    PyObject *item = PyTuple_GetItem(pobj, i);

    if (PyTuple_Check(item))
    {
      cols = PyTuple_Size(item);
      if (cols == sub_item_count || sub_item_count == -1)
      {
        sub_item_count = cols;
        for (Py_ssize_t j = 0; j < cols; j++)
        {
          PyObject *sub_item = PyTuple_GetItem(item, j);
          if (PyTuple_Check(sub_item))
          {
            is_matrix = false;
            break;
          }
        }
      }
      else
      {
        is_matrix = false;
        break;
      }
    }
    else
    {
      is_matrix = false;
    }
    if (!is_matrix)
      break;
  }
  return is_matrix;
}
Matrix Python_GetMatrix_Val(PyObject *pobj, Py_ssize_t cols, Py_ssize_t rows)
{
  Matrix m(rows, cols);

  for (Py_ssize_t i = 0; i < rows; i++)
  {
    PyObject *item = PyTuple_GetItem(pobj, i);
    for (Py_ssize_t j = 0; j < cols; j++)
    {
      PyObject *sub_item = PyTuple_GetItem(item, j);
      if (PyLong_Check(sub_item))
      {
        int overflow;
        long long value = PyLong_AsLongLongAndOverflow(sub_item, &overflow);
        if (overflow != 0)
        {
          throw std::runtime_error("Overflow when unpacking long");
        }
        m(i, j) = (double)value;
      }
      else if (PyFloat_Check(sub_item))
      {
        int overflow;
        double val = PyFloat_AS_DOUBLE(sub_item);
        long value = PyLong_AsLongLongAndOverflow(sub_item, &overflow);
        m(i, j) = val;
      }
      else if (PyBool_Check(sub_item))
      {
        int overflow;
        long long value = PyLong_AsLongLongAndOverflow(sub_item, &overflow);
        if (overflow != 0)
        {
          throw std::runtime_error("Overflow when unpacking long");
        }
        m(i, j) = (double)((value == 0) ? true : false);
      }
      else
      {
        printf("sub_item Unknown\n");
      }
    }
  }
  return m;
}

Cell Python_GetTuple_Val(PyObject *pobj)
{
  Py_ssize_t count = PyTuple_Size(pobj);
  size_t cout_tuple = static_cast<size_t>(count);
  Cell m(1, count);

  for (Py_ssize_t i = 0; i < count; i++)
  {
    PyObject *item = PyTuple_GetItem(pobj, i);
    if (PyLong_Check(item))
    {
      int overflow;
      long long value = PyLong_AsLongLongAndOverflow(item, &overflow);
      if (overflow != 0)
      {
        throw std::runtime_error("Overflow when unpacking long");
      }
      m(0, i) = (double)value;
    }
    else if (PyFloat_Check(item))
    {
      double val = PyFloat_AS_DOUBLE(item);
      m(0, i) = val;
    }
    else if (PyBool_Check(item))
    {
      int overflow;
      long long value = PyLong_AsLongLongAndOverflow(item, &overflow);
      if (overflow != 0)
      {
        throw std::runtime_error("Overflow when unpacking long");
      }
      m(0, i) = (double)((value == 0) ? true : false);
    }
    else if (PyBytes_Check(item) || PyUnicode_Check(item))
    {
      if (PyBytes_Check(item))
      {
        size_t size = PyBytes_GET_SIZE(item);
        m(0, i) = std::string(PyBytes_AS_STRING(item), size);
      }
      else if (PyUnicode_Check(item))
      {
        Py_ssize_t size;
        const char *data = PyUnicode_AsUTF8AndSize(item, &size);
        if (!data)
        {
          throw std::runtime_error("error unpacking string as utf-8");
        }
        m(0, i) = std::string(data, static_cast<size_t>(size));
      }
    }
    else
    {
      printf("Unknown\n");
    }
  }
  return m;
}
static string_vector
get_invoke_list(void *jobj_arg)
{
  PyObject *pobj = TO_PYTHON_OBJECT(jobj_arg);

  std::list<std::string> name_list;
  PyObject *dict = pobj->ob_type->tp_dict;
  PyObject *items = PyDict_Items(dict);
  int nitems = PyList_Size(items);
  for (int k = 0; k < nitems; ++k)
  {
    PyObject *obj;
    obj = PyList_GetItem(items, k);
    if (PyTuple_Check(obj))
    {
      PyObject *it1 = PyTuple_GetItem(obj, 0);
      PyObject *it2 = PyTuple_GetItem(obj, 1);
      std::string c_name1 = PyBytes_AsString(PyUnicode_AsASCIIString(it1));
      if (c_name1.at(0) != '_') //only public
      {
        name_list.push_back(c_name1);
      }
    }
  }
  octave_set_default_fpucw();

  string_vector v(name_list);

  return v.sort(true);
}

static PyObject *
make_python_index(const octave_value_list &idx)
{

  PyObject *retval = PyTuple_New(idx.length());

  for (int i = 0; i < idx.length(); i++)
    try
    {
      idx_vector v = idx(i).index_vector();
      PyObject *pArgs = PyTuple_New(v.length());
      for (int k = 0; k < v.length(); k++)
      {
        PyTuple_SetItem(pArgs, k, Py_BuildValue("i", (v(k))));
        check_exception();
      }
      PyTuple_SetItem(retval, i, Py_BuildValue("o", *pArgs));
      check_exception();
    }
    catch (octave::index_exception &e)
    {
      e.set_pos_if_unset(idx.length(), i + 1);
      throw;
    }
  return retval;
}
static octave_value
get_array_elements(PyObject *pobj,
                   const octave_value_list &idx)
{
  octave_value retval;
  PyObject *python_idx = make_python_index(idx);

  octave_set_default_fpucw();

  return retval;
}

static octave_value python_pack(PyObject *pobj)
{
  octave_value retval;
  if (!pobj)
    retval = Matrix();

  while (retval.is_undefined())
  {
    if (PyLong_Check(pobj))
    {
      int overflow;
      long long value = PyLong_AsLongLongAndOverflow(pobj, &overflow);
      if (overflow != 0)
      {
        throw std::runtime_error("Overflow when unpacking long");
      }
      retval = (long)value;
      break;
    }
    else if (PyFloat_Check(pobj))
    {
      double val = PyFloat_AsDouble(pobj);
      retval = val;
      break;
    }
    else if (PyBool_Check(pobj))
    {
      int overflow;
      long long value = PyLong_AsLongLongAndOverflow(pobj, &overflow);
      if (overflow != 0)
      {
        throw std::runtime_error("Overflow when unpacking long");
      }
      retval = (value == 0) ? true : false;
      break;
    }
    else if (PyBytes_Check(pobj) || PyUnicode_Check(pobj))
    {
      if (PyBytes_Check(pobj))
      {
        size_t size = PyBytes_GET_SIZE(pobj);
        retval = std::string(PyBytes_AS_STRING(pobj), size);
      }
      if (PyUnicode_Check(pobj))
      {
        Py_ssize_t size;
        const char *data = PyUnicode_AsUTF8AndSize(pobj, &size);
        if (!data)
        {
          throw std::runtime_error("error unpacking string as utf-8");
        }
        retval = std::string(data, static_cast<size_t>(size));
      }
      break;
    }
    else if (PyTuple_Check(pobj))
    {
      Py_ssize_t cols;
      Py_ssize_t rows;
      bool ismat = Python_IsMatrix(pobj, cols, rows);
      if (ismat)
      {
        Matrix m = Python_GetMatrix_Val(pobj, cols, rows);
        retval = m;
      }
      else
      {
        Cell m = Python_GetTuple_Val(pobj);
        retval = m;
      }
      break;
    }
    else if (PyList_Check(pobj))
    {
      Py_ssize_t count = PyList_Size(pobj);
      Cell m(1, count);
      for (Py_ssize_t i = 0; i < count; i++)
      {
        PyObject *item = PyList_GetItem(pobj, i);
        if (PyFloat_Check(item))
        {
          double val = PyFloat_AS_DOUBLE(item);
          m(0, i) = val;
        }
        else if (PyLong_Check(item))
        {
          int overflow;
          long long value = PyLong_AsLongLongAndOverflow(item, &overflow);
          if (overflow != 0)
          {
            throw std::runtime_error("Overflow when unpacking long");
          }
          m(0, i) = (double)value;
        }
        else if (PyBool_Check(item))
        {
          int overflow;
          long long value = PyLong_AsLongLongAndOverflow(item, &overflow);
          if (overflow != 0)
          {
            throw std::runtime_error("Overflow when unpacking long");
          }
          m(0, i) = (double)((value == 0) ? true : false);
        }
        else if (PyBytes_Check(item) || PyUnicode_Check(item))
        {
          if (PyBytes_Check(item))
          {
            size_t size = PyBytes_GET_SIZE(item);
            m(0, i) = std::string(PyBytes_AS_STRING(item), size);
          }
          else if (PyUnicode_Check(item))
          {
            Py_ssize_t size;
            const char *data = PyUnicode_AsUTF8AndSize(item, &size);
            if (!data)
            {
              throw std::runtime_error("error unpacking string as utf-8");
            }
            m(0, i) = std::string(data, static_cast<size_t>(size));
          }
        }
        else
        {
          printf("python_pack Unknown\n");
        }
      }

      retval = octave_value(m);
      break;
    }
    else
    {
      retval = octave_value(new octave_python(pobj));
      break;
    }
  }

  return retval;
}

static bool
python_unpack(const octave_value &val, PyObject **pobj)
{
  bool found = true;
  bool Vpython_matrix_autoconversion = false;
  if (val.ispython())
  {
    octave_python *ovj = TO_PYTHON(val);
    *pobj = (PyObject *)TO_PYTHON_OBJECT(ovj->to_python());
  }
  else if (val.is_string())
  {
    std::string s = val.string_value();

    *pobj = Py_BuildValue("s", s.c_str());
  }

  else if (val.iscellstr())
  {
    const Array<std::string> str_arr = val.cellstr_value();
    const octave_idx_type n = str_arr.numel();

    PyObject *pArgs = PyTuple_New(n);

    for (octave_idx_type i = 0; i < n; i++)
    {
      PyTuple_SetItem(pArgs, i, Py_BuildValue("s", (str_arr(i).c_str())));
    }

    *pobj = pArgs;
  }
  else if (val.numel() > 1 && val.dims().isvector())
  {
    printf(">>>>>>>>>>>>>\n");
    if (val.iscell())
    {

      Cell cell1 = val.cell_value();
      octave_idx_type nel = cell1.numel();
      *pobj = PyTuple_New(nel);

      for (octave_idx_type i = 0; i < nel; i++)
      {
        octave_value v = cell1(i);
        if (v.is_double_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("d", v.double_value()));
        else if (v.islogical())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("i", v.bool_value()));
        else if (v.isfloat())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("f", v.float_value()));
        else if (v.is_int8_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("b", v.int8_scalar_value()));
        else if (v.is_uint8_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("B", v.uint8_scalar_value()));
        else if (v.is_int16_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("h", v.int16_scalar_value()));
        else if (v.is_uint16_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("H", v.uint16_scalar_value()));
        else if (v.is_int32_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("i", v.int32_scalar_value()));
        else if (v.is_uint32_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("I", v.uint32_scalar_value()));
        else if (v.is_int64_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("L", v.int64_scalar_value()));
        else if (v.is_uint64_type())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("K", v.uint64_scalar_value()));
        else if (v.is_string())
          PyTuple_SetItem(*pobj, i, Py_BuildValue("s", v.string_value().c_str()));
      }
    }
    else
    {

#define UNBOX_PRIMITIVE_ARRAY(METHOD_T, TYPE_T, OCTAVE_T, PYTHON_T) \
  do                                                                \
  {                                                                 \
    const OCTAVE_T##NDArray v = val.METHOD_T##array_value();        \
    const TYPE_T *item = v.data();                                  \
    octave_idx_type nel = v.numel();                                \
    *pobj = PyTuple_New(nel);                                       \
    for (octave_idx_type i = 0; i < nel; i++)                       \
    {                                                               \
      PyTuple_SetItem(*pobj, i, Py_BuildValue(PYTHON_T, item[i]));  \
    }                                                               \
  } while (0)

      if (val.is_double_type())
        UNBOX_PRIMITIVE_ARRAY(, double, , "d");
      else if (val.islogical())
        UNBOX_PRIMITIVE_ARRAY(bool_, bool, bool, "i");
      else if (val.isfloat())
        UNBOX_PRIMITIVE_ARRAY(float_, float, Float, "f");
      else if (val.is_int8_type())
        UNBOX_PRIMITIVE_ARRAY(int8_, octave_int<signed char>, int8, "b");
      else if (val.is_uint8_type())
        UNBOX_PRIMITIVE_ARRAY(uint8_, octave_int<unsigned char>, uint8, "B");
      else if (val.is_int16_type())
        UNBOX_PRIMITIVE_ARRAY(int16_, octave_int<short int>, int16, "h");
      else if (val.is_uint16_type())
        UNBOX_PRIMITIVE_ARRAY(uint16_, octave_int<short unsigned int>, uint16, "H");
      else if (val.is_int32_type())
        UNBOX_PRIMITIVE_ARRAY(int32_, octave_int<int>, int32, "i");
      else if (val.is_uint32_type())
        UNBOX_PRIMITIVE_ARRAY(uint32_, octave_int<unsigned int>, uint32, "I");
      else if (val.is_int64_type())
        UNBOX_PRIMITIVE_ARRAY(int64_, octave_int<long int>, int64, "L");
      else if (val.is_uint64_type())
        UNBOX_PRIMITIVE_ARRAY(uint64_, octave_int<long unsigned int>, uint64, "K");

#undef UNBOX_PRIMITIVE_ARRAY
    }
  }
  else if (val.is_real_scalar() || val.is_bool_scalar())
  {

#define UNBOX_PRIMITIVE_SCALAR(OCTAVE_T, METHOD_T, PYTHON_T) \
  do                                                         \
  {                                                          \
    const OCTAVE_T ov = val.METHOD_T##_value();              \
    *pobj = Py_BuildValue(PYTHON_T, ov);                     \
  } while (0)

    if (val.is_double_type())
      UNBOX_PRIMITIVE_SCALAR(double, double, "d");
    else if (val.islogical())
      UNBOX_PRIMITIVE_SCALAR(bool, bool, "i");
    else if (val.isfloat())
      UNBOX_PRIMITIVE_SCALAR(float, float, "f");
    else if (val.is_int8_type())
      UNBOX_PRIMITIVE_SCALAR(int8_t, int8_scalar, "b");
    else if (val.is_uint8_type())
      UNBOX_PRIMITIVE_SCALAR(uint8_t, uint8_scalar, "B");
    else if (val.is_int16_type())
      UNBOX_PRIMITIVE_SCALAR(int16_t, int16_scalar, "h");
    else if (val.is_uint16_type())
      UNBOX_PRIMITIVE_SCALAR(uint16_t, uint16_scalar, "H");
    else if (val.is_int32_type())
      UNBOX_PRIMITIVE_SCALAR(int32_t, int32_scalar, "i");
    else if (val.is_uint32_type())
      UNBOX_PRIMITIVE_SCALAR(uint32_t, uint32_scalar, "I");
    else if (val.is_int64_type())
      UNBOX_PRIMITIVE_SCALAR(int64_t, int64_scalar, "L");
    else if (val.is_uint64_type())
      UNBOX_PRIMITIVE_SCALAR(uint64_t, uint64_scalar, "K");

#undef UNBOX_PRIMITIVE_SCALAR
  }
  else if (val.isempty())
  {
    *pobj = nullptr;
  }
  else if (!Vpython_matrix_autoconversion && ((val.is_real_matrix() && (val.rows() == 1 || val.columns() == 1)) || val.is_range()))
  {
    Matrix m = val.matrix_value();

    *pobj = Py_BuildValue("(d)", m.numel());
  }
  else if (val.is_matrix_type() && val.isreal())
  {
    Matrix m = val.matrix_value();

    octave_idx_type rows = m.rows();
    octave_idx_type cols = m.columns();

    *pobj = PyTuple_New(rows);
    for (octave_idx_type i = 0; i < rows; i++)
    {
      PyObject *ai = PyTuple_New(cols);
      for (octave_idx_type j = 0; j < cols; j++)
      {
        octave_value v = m(i, j);

        if (v.is_double_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("d", v.double_value()));
        else if (val.islogical())
          PyTuple_SetItem(ai, j, Py_BuildValue("i", v.bool_value()));
        else if (val.isfloat())
          PyTuple_SetItem(ai, j, Py_BuildValue("f", v.float_value()));
        else if (val.is_int8_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("b", v.int8_scalar_value()));
        else if (val.is_uint8_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("B", v.uint8_scalar_value()));
        else if (val.is_int16_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("h", v.int16_scalar_value()));
        else if (val.is_uint16_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("H", v.uint16_scalar_value()));
        else if (val.is_int32_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("i", v.int32_scalar_value()));
        else if (val.is_uint32_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("I", v.uint32_scalar_value()));
        else if (val.is_int64_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("L", v.int64_scalar_value()));
        else if (val.is_uint64_type())
          PyTuple_SetItem(ai, j, Py_BuildValue("K", v.uint64_scalar_value()));
      }
      PyTuple_SetItem(*pobj, i, ai);
    }
  }
  else
  {
    printf("val unknown!\n");
  }

  return found;
}

static bool
python_unpack(const octave_value_list &args,
              PyObject **pArgs)
{
  bool found = true;
  *pArgs = PyTuple_New(args.length());
  for (int i = 0; i < args.length(); i++)
  {
    PyObject *pArg;
    found = python_unpack(args(i), &pArg);
    if (!found)
      break;
    PyTuple_SetItem(*pArgs, i, pArg);
  }
  return found;
}

bool octave_python::is_instance_of(const std::string &cls_name) const
{
#if defined(HAVE_PYTHON)
  PyObject *obj = TO_PYTHON_OBJECT(this->python_object);
  return cls_name.compare(Py_TYPE(obj)->tp_name) == 0 ? true : false;
#else
  octave_unused_parameter(cls_name);
  panic_impossible();

#endif
}

octave_python::octave_python(void)
    : octave_base_value(),
      python_object(nullptr)
{
#if !defined(HAVE_PYTHON)

  err_disabled_feature("Python Objects", "Python");

#endif
}

octave_python::octave_python(const voidptr &pobj)
    : octave_base_value(),
      python_object(pobj)
{
#if defined(HAVE_PYTHON)
  init(pobj);
#else

  octave_unused_parameter(pobj);

  err_disabled_feature("Python Objects", "Python");

#endif
}

int octave_python::t_id(-1);

const std::string octave_python::t_name("octave_python");

void octave_python::register_type(octave::type_info &ti)
{
#if defined(HAVE_PYTHON)
  t_id = ti.register_type(octave_python::t_name, "<unknown>",
                          octave_value(new octave_python()));
#else

  octave_unused_parameter(ti);

#endif
}

dim_vector
octave_python::dims(void) const
{
#if defined(HAVE_PYTHON)
  return dim_vector(1, 1);

#else
  panic_impossible();

#endif
}

octave_value_list
octave_python::subsref(const std::string &type,
                       const std::list<octave_value_list> &idx, int nargout)
{
#if defined(HAVE_PYTHON)
  octave_value_list retval;
  int skip = 1;

  switch (type[0])
  {
  case '.':
    if (type.length() > 1 && type[1] == '(')
    {
      octave_value_list ovl;
      count++;
      ovl(1) = octave_value(this);
      ovl(0) = (idx.front())(0);
      auto it = idx.begin();
      ovl.append(*++it);
      retval = FpythonMethod(ovl, 1);
      skip++;
    }
    else
    {
      octave_value_list ovl;
      count++;
      ovl(0) = octave_value(this);
      ovl(1) = (idx.front())(0);
      retval = F__python_get__(ovl, 1);
    }
    break;
  case '(':
    retval = get_array_elements(TO_PYTHON_OBJECT(to_python()), idx.front());
    break;
  default:
    error("subsref: Python object cannot be indexed with %c", type[0]);
    break;
  }

  if (idx.size() > 1 && type.length() > 1)
    retval = retval(0).next_subsref(nargout, type, idx, skip);

  return retval;

#else

  octave_unused_parameter(type);
  octave_unused_parameter(idx);
  octave_unused_parameter(nargout);
  panic_impossible();

#endif
}

octave_value
octave_python::subsasgn(const std::string &type,
                        const std::list<octave_value_list> &idx,
                        const octave_value &rhs)
{
#if defined(HAVE_PYTHON)
  octave_value retval;

  switch (type[0])
  {
  case '.':
    if (type.length() == 1)
    {
      octave_value_list ovl;
      count++;
      ovl(0) = octave_value(this);
      ovl(1) = (idx.front())(0);
      ovl(2) = rhs;
      F__python_set__(ovl);

      count++;
      retval = octave_value(this);
    }
    else if (type.length() > 2 && type[1] == '(')
    {
      std::list<octave_value_list> new_idx;
      auto it = idx.begin();
      new_idx.push_back(*it++);
      new_idx.push_back(*it++);
      octave_value_list u = subsref(type.substr(0, 2), new_idx, 1);

      std::list<octave_value_list> next_idx(idx);
      next_idx.erase(next_idx.begin());
      next_idx.erase(next_idx.begin());
      u(0).subsasgn(type.substr(2), next_idx, rhs);

      count++;
      retval = octave_value(this);
    }
    else if (type[1] == '.')
    {
      octave_value_list u = subsref(type.substr(0, 1), idx, 1);

      std::list<octave_value_list> next_idx(idx);
      next_idx.erase(next_idx.begin());
      u(0).subsasgn(type.substr(1), next_idx, rhs);

      count++;
      retval = octave_value(this);
    }
    else
      error("invalid indexing/assignment on Python object");
    break;

  case '(':

    count++;
    retval = octave_value(this);
    break;

  default:
    error("Python object cannot be indexed with %c", type[0]);
    break;
  }

  return retval;

#else

  octave_unused_parameter(type);
  octave_unused_parameter(idx);
  octave_unused_parameter(rhs);

  panic_impossible();

#endif
}

string_vector
octave_python::map_keys(void) const
{
#if defined(HAVE_PYTHON)
  return get_invoke_list(to_python());
#else

  panic_impossible();

#endif
}

void octave_python::print(std::ostream &os, bool)
{
  print_raw(os);
  newline(os);
}

void octave_python::print_raw(std::ostream &os, bool) const
{
  os << "<Python object: " << python_classname << '>';
}

bool octave_python::save_ascii(std::ostream & /* os */)
{
  warning("save: unable to save Python objects, skipping");

  return true;
}

bool octave_python::load_ascii(std::istream & /* is */)
{
  // Silently skip over Python object that was not saved
  return true;
}

bool octave_python::save_binary(std::ostream & /* os */, bool /* save_as_floats */)
{
  warning("save: unable to save python objects, skipping");

  return true;
}

bool octave_python::load_binary(std::istream & /* is */, bool /* swap*/,
                                octave::mach_info::float_format /* fmt */)
{
  // Silently skip over python object that was not saved
  return true;
}

bool octave_python::save_hdf5(octave_hdf5_id /* loc_id */, const char * /* name */,
                              bool /* save_as_floats */)
{
  warning("save: unable to save python objects, skipping");

  return true;
}

bool octave_python::load_hdf5(octave_hdf5_id /* loc_id */, const char * /* name */)
{
  // Silently skip object that was not saved
  return true;
}

octave_value
octave_python::do_pythonMethod(const std::string &name,
                               const octave_value_list &args)
{
#if defined(HAVE_PYTHON)
  PyObject *jobjs;
  python_unpack(args, &jobjs);

  PyObject *pFunc = PyObject_GetAttrString(static_cast<PyObject *>(python_object), name.c_str());
  if (pFunc == NULL)
  {
    PyErr_Print();
    error("[Python] Failed to PyObject_GetAttrString %s", name.c_str());
  }
  PyObject *pclass = PyObject_CallObject(pFunc, jobjs);

  Py_DECREF(jobjs);

  PyObject *err = PyErr_Occurred();

  if (err != NULL || pclass == NULL)
  {
    PyErr_Print();
    error("[Python] Failed to execute method %s", name.c_str());
  }
  else
  {
    return python_pack(pclass);
  }
#else

  octave_unused_parameter(name);
  octave_unused_parameter(args);

  panic_impossible();

#endif
}

octave_value
octave_python::do_pythonObject(const octave_python *module,
                               const std::string &classname,
                               const octave_value_list &args)
{
#if defined(HAVE_PYTHON)
  PyObject *pclass = PyObject_GetAttrString(TO_PYTHON_OBJECT(module->to_python()), classname.c_str());
  if (pclass == NULL)
  {
    PyObject *type = PyErr_Occurred();
    PyErr_Print();
    error("[Python] PyObject_GetAttrString Failed :%s\n", classname.c_str());
  }
  PyObject *jobjs;
  python_unpack(args, &jobjs);

  PyObject *pinstance = PyObject_Call(pclass, jobjs, NULL);
  Py_DECREF(jobjs);
  if (pinstance == NULL)
  {
    PyErr_Print();
    error("[Python] Failed to create object %s", classname.c_str());
  }
  else
  {
    return octave_value(new octave_python(pinstance));
  }

#else

  octave_unused_parameter(name);
  octave_unused_parameter(args);
  panic_impossible();

#endif
}

octave_value
octave_python::do_python_get(const std::string &name)
{
#if defined(HAVE_PYTHON)
  octave_value retval;

  PyObject *pFunc = PyObject_GetAttrString(static_cast<PyObject *>(python_object), name.c_str());
  octave_set_default_fpucw();

  retval = python_pack(pFunc);
  return retval;

#else

  octave_unused_parameter(name);

  panic_impossible();

#endif
}

octave_value
octave_python::do_python_set(const std::string &name, const octave_value &val)
{
#if defined(HAVE_PYTHON)
  PyObject *pobj;
  octave_value retval;
  if (python_unpack(val, &pobj))
  {
    int ret = PyObject_SetAttrString(static_cast<PyObject *>(python_object), name.c_str(), pobj);
    retval = python_pack(pobj);
  }

  octave_set_default_fpucw();
  return retval;
#else

  octave_unused_parameter(name);
  octave_unused_parameter(val);

  panic_impossible();

#endif
}

void octave_python::init(void *jobj_arg)
{
#if defined(HAVE_PYTHON)
  PyObject *object = TO_PYTHON_OBJECT(jobj_arg);
  const char *func_name = PyEval_GetFuncName(object);
  python_classname = func_name;
#else
  octave_unused_parameter(jobj_arg);

  panic_impossible();

#endif
}

void octave_python::release(void)
{
#if defined(HAVE_PYTHON)

#else

  panic_impossible();

#endif
}

// DEFUN blocks below must be outside of HAVE_PYTHON block so that documentation
// strings are always available, even when functions are not.

DEFUN(__python_init__, , ,
      doc
      : /* -*- texinfo -*-
@deftypefn {} {} __python_init__ ()
Internal function used @strong{only} when debugging Python interface.

Function will directly call initialize_python to create an instance of a JVM.
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)
  octave_python_init();
#else

  err_disabled_feature("__python_init__", "Python");

#endif
}

DEFUN(__python_exit__, , ,
      doc
      : /* -*- texinfo -*-
@deftypefn {} {} __python_exit__ ()
Internal function used @strong{only} when debugging Python interface.

Function will directly call terminate_jvm to destroy the current JVM
instance.
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)
  octave_python_free();
#else

  err_disabled_feature("__python_exit__", "Python");

#endif
}

DEFUN(pythonImport, args, ,
      doc
      : /* -*- texinfo -*-
@deftypefn  {} {@var{pobj} =} pythonImport (@var{packagename})
@deftypefnx {} {@var{pobj} =} pythonObject (@var{packagename}, @var{arg1}, @dots{})
Import a Python package @var{packagename}

@example
@group
pythonImport ("sys")
pythonImport ("numpy")
@end group
@end example

@seealso{pythonObject, pythonMethod, pythonArray}
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)
  octave_python_init();
  if (args.length() == 0)
    print_usage();

  std::string modname = args(0).xstring_value("PythonImport: Pacakge must be a string");

  PyObject *mod = PyImport_ImportModule(modname.c_str());
  if (mod == NULL)
  {
    PyErr_Print();
    error("[Python] Failed to load module %s", modname.c_str());
  }
  else
  {
    return octave_value(new octave_python(mod));
  }

#else

  octave_unused_parameter(args);

  err_disabled_feature("pythonPackage", "Python");

#endif
}

DEFUN(pythonObject, args, ,
      doc
      : /* -*- texinfo -*-
@deftypefn  {} {@var{pobj} =} pythonObject (@var{classname})
@deftypefnx {} {@var{pobj} =} pythonObject (@var{classname}, @var{arg1}, @dots{})
Create a Python object of class @var{classsname}, by calling the class
constructor with the arguments @var{arg1}, @dots{}

The first example below creates an uninitialized object, while the second
example supplies an initial argument to the constructor.

@example
@group
x = pythonObject ("python.lang.StringBuffer")
x = pythonObject ("python.lang.StringBuffer", "Initial string")
@end group
@end example

@seealso{pythonMethod, pythonArray}
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)
  octave_python_init();
  if (args.length() <= 1)
    print_usage();

  std::string classname = args(0).xstring_value("PythonObject: CLASSNAME must be a string");

  if (!args(1).ispython())
  {
    error("[Python] The second parameter must be a Python module");
  }
  octave_python *module = TO_PYTHON(args(1));
  octave_value retval;

  octave_value_list tmp;
  for (int i = 2; i < args.length(); i++)
    tmp(i - 2) = args(i);
  return ovl(octave_python::do_pythonObject(module, classname, tmp));

#else

  octave_unused_parameter(args);

  err_disabled_feature("pythonObject", "Python");

#endif
}

/*
## The tests below merely check if pythonObject works at all.  Whether
## it works properly, i.e., creates the right values, is a matter of
## Python itself.  Create a Short and check if it really is a short, i.e.,
## whether it overflows.
%!testif HAVE_PYTHON; usepython ("jvm")
%! assert (pythonObject ("python.lang.Short", 40000).doubleValue < 0);
*/

DEFUN(pythonMethod, args, ,
      doc
      : /* -*- texinfo -*-
@deftypefn  {} {@var{ret} =} pythonMethod (@var{methodname}, @var{obj})
@deftypefnx {} {@var{ret} =} pythonMethod (@var{methodname}, @var{obj}, @var{arg1}, @dots{})
Invoke the method @var{methodname} on the Python object @var{obj} with the
arguments @var{arg1}, @dots{}.

For static methods, @var{obj} can be a string representing the fully
qualified name of the corresponding class.

When @var{obj} is a regular Python object, structure-like indexing can be
used as a shortcut syntax.  For instance, the two following statements are
equivalent

@example
@group
  ret = pythonMethod ("method1", x, 1.0, "a string")
  ret = x.method1 (1.0, "a string")
@end group
@end example

@code{pythonMethod} returns the result of the method invocation.

@seealso{methods, pythonObject}
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)
  octave_python_init();
  if (args.length() < 2)
    print_usage();

  std::string methodname = args(0).xstring_value("pythonMethod: METHODNAME must be a string");

  octave_value retval;

  octave_value_list tmp;
  for (int i = 2; i < args.length(); i++)
    tmp(i - 2) = args(i);
  if (args(1).ispython())
  {
    octave_python *object = TO_PYTHON(args(1));
    retval = object->do_pythonMethod(methodname, tmp);
  }
  else
    error("[pythonMethod] The second parameter[OBJ] must be a Python object");

  return retval;

#else

  octave_unused_parameter(args);

  err_disabled_feature("pythonMethod", "Python");

#endif
}

/*
%!testif HAVE_PYTHON; usepython ("jvm")
%! jver = pythonMethod ("getProperty", "python.lang.System", "python.version");
%! jver = strsplit (jver, ".");
%! if (numel (jver) > 1)
%!   assert (isfinite (str2double (jver{1})));
%!   assert (isfinite (str2double (jver{2})));
%! else
%!   assert (isfinite (str2double (jver{1})));
%! endif
*/

DEFUN(__python_get__, args, ,
      doc
      : /* -*- texinfo -*-
@deftypefn {} {@var{val} =} __python_get__ (@var{obj}, @var{name})
Get the value of the field @var{name} of the Python object @var{obj}.

For static fields, @var{obj} can be a string representing the fully
qualified name of the corresponding class.

When @var{obj} is a regular Python object, structure-like indexing can be used
as a shortcut syntax.  For instance, the two following statements are
equivalent

@example
@group
  __python_get__ (x, "field1")
  x.field1
@end group
  @end example

@seealso{__python_set__, pythonMethod, pythonObject}
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)
  if (args.length() != 2)
    print_usage();
  std::string name = args(1).xstring_value("__python_get__: NAME must be a string");

  octave_python_init();
  octave_value retval;

  if (args(0).ispython())
  {
    octave_python *pobj = TO_PYTHON(args(0));
    retval = pobj->do_python_get(name);
  }
  else
    error("__python_get__: OBJ must be a Python object or a string");

  return retval;
#else

  octave_unused_parameter(args);

  err_disabled_feature("__python_get__", "Python");

#endif
}

DEFUN(__python_set__, args, ,
      doc
      : /* -*- texinfo -*-
@deftypefn {} {@var{obj} =} __python_set__ (@var{obj}, @var{name}, @var{val})
Set the value of the field @var{name} of the Python object @var{obj} to
@var{val}.

For static fields, @var{obj} can be a string representing the fully
qualified named of the corresponding Python class.

When @var{obj} is a regular Python object, structure-like indexing can be
used as a shortcut syntax.  For instance, the two following statements are
equivalent

@example
@group
  __python_set__ (x, "field1", val)
  x.field1 = val
@end group
@end example

@seealso{__python_get__, pythonMethod, pythonObject}
@end deftypefn */
)
{
#if defined(HAVE_PYTHON)

  if (args.length() != 3)
    print_usage();

  std::string name = args(1).xstring_value("__python_get__: NAME must be a string");

  octave_python_init();

  octave_value retval;

  if (args(0).ispython())
  {
    octave_python *pobj = TO_PYTHON(args(0));
    retval = pobj->do_python_set(name, args(2));
  }
  else
    error("__python_get__: OBJ must be a Python object or a string");

  return retval;
#else

  octave_unused_parameter(args);

  err_disabled_feature("__python_set__", "Python");

#endif
}

// Outside of #if defined (HAVE_PYTHON) because it is desirable to be able
// to test for the presence of a Python object without having Python
// installed.

DEFUN(ispython, args, ,
      doc
      : /* -*- texinfo -*-
@deftypefn {} {} ispython (@var{x})
Return true if @var{x} is a Python object.
@seealso{class, typeinfo, isa, pythonObject}
@end deftypefn */
)
{
  if (args.length() != 1)
    print_usage();

  return ovl(args(0).ispython());
}
