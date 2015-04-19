#include "Python.h"

/*
NOTES:
- Python.h modifies assert() so it does not trigger program termination on failure. exit() terminates the interpreter.
- small memory allocations assumed to work.
- ndarrays not allowed to contain objects with structure.

Common brief variable names and meanings
i, j : for-loop counters
len, n : the length of a list/tuple/array/string/etc object
lobj : list Python object
obj : generic Python object
robj : the Python object that will be returned by a function
sobj : string Python object
tobj : temporary Python object
*/

#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define EXCEPTION_RAISED -1
#define BIG_ENDIAN ">"
#define LITTLE_ENDIAN "<"

#define LABRAD_BOOL_SIZE 1
#define LABRAD_INT_SIZE 4
#define LABRAD_LONG_SIZE 8
#define LABRAD_FLOAT_SIZE 8

// supported internal data types
#define EMPTY 0
#define BOOL 1
#define UINT31 2
#define INT32 3
#define UINT32 4
#define FLOAT64 5
#define COMPLEX128 6
#define FLOAT64_SU 7
#define COMPLEX128_SU 8
#define STRING 9
#define CLUSTER 10
#define LIST 11
#define NDARRAY 12
#define NDARRAY_SU 13
#define TIME 14
#define ERROR 15

// COBJ is an all-encompassing structure containing a wide enough variety of elements to represent any of the above types
typedef struct cobj COBJ;

struct cobj {
    int type; // type of this cobj
    int subtype; // if NDARRAY(_SU), type of each element in array, if ERROR, specific type of ERROR
    int istype; // FALSE unless structure contains only type information
    int i; // good for BOOL, UINT31, INT32
    unsigned int n; // good for UINT32, length cobj_arr, length string, length dim_arr
    double x, y; // FLOAT64 and COMPLEX128 data
    char *str; // STRING, unit name, and ERROR data
    COBJ **cobj_arr; // sub cobjs contained in CLUSTER or LIST
    int *dim_arr; // dimensions of NDARRAY(_SU)
    void *arr; // data of NDARRAY(_SU)
    int isarrowned; // FALSE unless arr has be recreated to be a different type, usually arr is just a pointer to data in the pyobject.
    long long int secs, usecs; // TIME data
};

// the following is a simplification of the real numpy PyArrayObject, use with caution
typedef struct PyArrayObject {
    PyObject_HEAD
    char *data;
    int nd;
    int *dimensions;
    int *strides;
    PyObject *base;
    void *descr;
    int flags;
    PyObject *weakreflist;
} PyArrayObject;

// global variables, initialized by first function call (which will take much longer than subsequent calls)
int ft_gv_initialized = FALSE;
int ft_gv_system_big_endian = FALSE;
int ft_gv_send_big_endian = TRUE;
PyObject *ft_gv_0 = NULL;
PyObject *ft_gv_1 = NULL;
PyObject *ft_gv_2 = NULL;
PyObject *ft_gv_int_num_bits = NULL;
PyObject *ft_gv_int_num_bits_minus_1 = NULL;
PyObject *ft_gv_INT_MIN = NULL;
PyObject *ft_gv_INT_MAX = NULL;
PyObject *ft_gv_UINT_MAX = NULL;
PyObject *ft_gv_numpy = NULL;
PyTypeObject *ft_gv_numpy_ndarray = NULL;
PyTypeObject *ft_gv_numpy_bool_ = NULL;
PyTypeObject *ft_gv_numpy_int32 = NULL;
PyTypeObject *ft_gv_numpy_uint32 = NULL;
PyTypeObject *ft_gv_numpy_float64 = NULL;
PyTypeObject *ft_gv_numpy_complex128 = NULL;
PyObject *ft_gv_labrad_types = NULL;
PyObject *ft_gv_labrad_units = NULL;
PyTypeObject *ft_gv_labrad_units_WithUnit = NULL;
PyTypeObject *ft_gv_labrad_units_Value = NULL;
PyTypeObject *ft_gv_labrad_units_Complex = NULL;
PyTypeObject *ft_gv_labrad_units_ValueArray = NULL;

// C equivalent of labrad.types.flatten, called in similar manner, although type tags must be strings
PyObject *ft_flatten(PyObject *self, PyObject *args, PyObject *keywds);

// functions to create an internal C representation (COBJ) of supported Python objects
COBJ *ft_create_cobj_from_pyobj(PyObject *obj);
    COBJ *ft_allocate_memory_for_cobj();
    COBJ *ft_create_empty_cobj();
    COBJ *ft_create_cobj_from_pybool(PyObject *obj);
    COBJ *ft_create_cobj_from_pyint_or_pylong(PyObject *obj);
    COBJ *ft_create_cobj_from_pyfloat(PyObject *obj);
    COBJ *ft_create_cobj_from_pycomplex(PyObject *obj);
    COBJ *ft_create_cobj_from_pyWithUnit(PyObject *obj);
        char *ft_get_unit_string_ptr(PyObject *obj);
        COBJ *ft_convert_to_standard_units(COBJ *cobj);
    COBJ *ft_create_cobj_from_pystring(PyObject *obj);
    COBJ *ft_create_cobj_from_pycluster(PyObject *obj);
    COBJ *ft_create_cobj_from_pylist(PyObject *obj);
    COBJ *ft_create_cobj_from_pyndarray(PyObject *obj);
    
    COBJ *ft_create_cobj_from_bool(char b);
    COBJ *ft_create_cobj_from_int32(int i);
    COBJ *ft_create_cobj_from_uint32(unsigned int n);
    COBJ *ft_create_cobj_from_float64(double x);
    COBJ *ft_create_cobj_from_complex128(double x, double y);

    COBJ *ft_create_cobj(int type, int istype);
    
    void ft_free_cobj(COBJ *cobj);
    
// functions to create a representation of the type of a COBJ
COBJ *ft_create_cobj_type(COBJ *cobj);
    COBJ *ft_create_list_type(COBJ *cobj);

// deprecated
// COBJ *ft_homogenize_list(COBJ *cobj);

// necessarily complex function that takes an object/type and a type and makes the first more general to encompass the second. eg: int, uint returns float since this
// is the smallest type able to represent every int and uint value. When the second argument contains placeholder values (such as v[]) it can be modified. eg: v[s], v[]
// will not only return v[s] but modify the second argument to be v[s].
COBJ *ft_upgrade_first_cobj_to_encompass_second(COBJ *cobj1, COBJ *cobj2);
    COBJ *ft_upgrade_first_subtype_to_encompass_second(COBJ *cobj1, COBJ *cobj2);
    COBJ *ft_convert_ndarray_to_list(COBJ *cobj);
    COBJ *ft_create_cobj_from_ndarray_i(COBJ *cobj, int i);
    int ft_size(COBJ *cobj);
    int ft_size_subtype(COBJ *cobj);
    
// functions to convert a type tag into a cobj type
COBJ *ft_create_cobj_tag_from_pystring(PyObject *tt);
    COBJ *ft_create_cobj_tag_from_char_buf(char *buf, int *iptr, int n);

// functions to initialize the global variables
int ft_initialize();
    int ft_system_supported();
    void ft_detect_system_endianness();
    int ft_set_send_endianness(PyObject *endianness);

// function to print a cobj
void ft_print_cobj(COBJ *cobj);
    void ft_print_type(COBJ *cobj);
    void ft_print_subtype(COBJ *cobj);
    void ft_print_type_text(int type);

// currently immature test function
PyObject *ft_test(PyObject *self, PyObject *args);

PyObject *ft_flatten(PyObject *self, PyObject *args, PyObject *keywds) {
    int i, n;
    PyObject *obj, *types, *endianness, *sobj, *lobj, *tobj/*, *robj*/;
    char *kwlist[] = {"obj", "types", "endianness", NULL};
    COBJ /* *cobj, *cobj_type,*/ *cobj_tag;
    
    if (!ft_gv_initialized) {
        if (ft_initialize() == EXCEPTION_RAISED) goto exception;
    }
    
    obj = types = endianness = sobj = lobj = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OO", kwlist, &obj, &types, &endianness)) goto exception;
    
    if (endianness != NULL) {
        if (ft_set_send_endianness(endianness) == EXCEPTION_RAISED) goto exception;
    }
    
    if (types == NULL) {
        types = sobj = PyString_FromString("?");
    }
    if (!PyList_Check(types)) {
        lobj = PyList_New(1);
        if (sobj == NULL) Py_INCREF(types);
        PyList_SET_ITEM(lobj, 0, types);
        types = lobj;
    }
    
    n = PyList_GET_SIZE(types);
    for (i=0; i<n; i++) {
        tobj = PyList_GET_ITEM(types, i);
        cobj_tag = ft_create_cobj_tag_from_pystring(tobj);
        if (cobj_tag == NULL) goto exception;
        ft_print_cobj(cobj_tag);
        ft_free_cobj(cobj_tag);
    }
    
    /*
    cobj = ft_create_cobj_from_pyobj(obj);
    cobj = ft_convert_ndarray_to_list(cobj);
    if (cobj == NULL) goto exception;
    ft_print_cobj(cobj);
    printf("\n");
    */
    
    /*
    cobj = ft_create_cobj_from_pyobj(obj);
    if (cobj == NULL) goto exception;
    ft_print_cobj(cobj);
    printf("\n");
    
    cobj_type = ft_create_cobj_type(cobj);
    if (cobj_type == NULL) {
        ft_free_cobj(cobj);
        goto exception;
    }
    ft_print_cobj(cobj_type);
    
    printf("\n----------------------------------------------------------------------------\n\n");
    ft_print_cobj(cobj);
    printf("\n----------------------------------------------------------------------------\n\n");
    
    cobj = ft_upgrade_first_cobj_to_encompass_second(cobj, cobj_type);
    if (cobj == NULL) {
        ft_free_cobj(cobj_type);
        goto exception;
    }
    ft_print_cobj(cobj);
    printf("\n");
    */
    
    // temporarily just clean up
    // ft_free_cobj(cobj);
    // ft_free_cobj(cobj_type);
    if (lobj != NULL) Py_DECREF(lobj);
    
    Py_INCREF(Py_None);
    return Py_None;
    // -------------------------

exception:
    
    if (lobj != NULL) Py_DECREF(lobj);
    
    return NULL;
}

COBJ *ft_create_cobj_from_pyobj(PyObject *obj) {
    COBJ *cobj;
    
    cobj = NULL;
    
    if (obj == Py_None) {
        cobj = ft_create_empty_cobj();
    }
    else if (PyBool_Check(obj)) {
        cobj = ft_create_cobj_from_pybool(obj);
    }
    else if (PyInt_Check(obj) || PyLong_Check(obj)) {
        cobj = ft_create_cobj_from_pyint_or_pylong(obj);
        if (cobj == NULL) goto exception;
    }
    else if (PyFloat_Check(obj)) {
        cobj = ft_create_cobj_from_pyfloat(obj);
    }
    else if (PyComplex_Check(obj)) {
        cobj = ft_create_cobj_from_pycomplex(obj);
    }
    else if (PyObject_TypeCheck(obj, ft_gv_labrad_units_WithUnit)) {
        cobj = ft_create_cobj_from_pyWithUnit(obj);
        if (cobj == NULL) goto exception;
    }
    else if (PyString_Check(obj)) {
        cobj = ft_create_cobj_from_pystring(obj);
    }
    else if (PyTuple_Check(obj)) {
        cobj = ft_create_cobj_from_pycluster(obj);
        if (cobj == NULL) goto exception;
    }
    else if (PyList_Check(obj)) {
        cobj = ft_create_cobj_from_pylist(obj);
        if (cobj == NULL) goto exception;
    }
    else if (PyObject_TypeCheck(obj, ft_gv_numpy_ndarray)) {
        cobj = ft_create_cobj_from_pyndarray(obj);
        if (cobj == NULL) goto exception;
    }
    
    if (cobj == NULL) {
        PyErr_SetString(PyExc_TypeError, "unsupported python object");
        goto exception;
    }
    
    return cobj;
    
exception:
    
    return NULL;
}

COBJ *ft_allocate_memory_for_cobj() {
    return (COBJ *)malloc(sizeof(COBJ));
}

COBJ *ft_create_empty_cobj() {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = EMPTY;
    cobj->istype = FALSE;
    
    return cobj;
}

COBJ *ft_create_cobj_from_pybool(PyObject *obj) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = BOOL;
    cobj->istype = FALSE;
    
    if (obj == Py_True) cobj->i = TRUE;
    else cobj->i = FALSE;
    
    return cobj;
}

COBJ *ft_create_cobj_from_pyint_or_pylong(PyObject *obj) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->istype = FALSE;
    
    if (PyObject_Compare(obj, ft_gv_0) < 0) {
        if (PyObject_Compare(obj, ft_gv_INT_MIN) < 0) {
            cobj->type = FLOAT64;
            cobj->x = PyLong_AsDouble(obj);
            if (PyErr_Occurred() != NULL) goto exception;
        }
        else {
            cobj->type = INT32;
            cobj->i = (int)PyInt_AS_LONG(obj);
        }
    }
    else if (PyObject_Compare(obj, ft_gv_UINT_MAX) > 0) {
        cobj->type = FLOAT64;
        cobj->x = PyLong_AsDouble(obj);
        if (PyErr_Occurred() != NULL) goto exception;
    }
    else if (PyObject_Compare(obj, ft_gv_INT_MAX) > 0) {
        cobj->type = UINT32;
        cobj->n = PyLong_AsUnsignedLong(obj);
    }
    else {
        cobj->type = UINT31;
        cobj->i = (int)PyInt_AS_LONG(obj);
    }
    
    return cobj;
    
exception:
    
    ft_free_cobj(cobj);
    
    return NULL;
}

COBJ *ft_create_cobj_from_pyfloat(PyObject *obj) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = FLOAT64;
    cobj->istype = FALSE;
    cobj->x = PyFloat_AS_DOUBLE(obj);
    
    return cobj;
}

COBJ *ft_create_cobj_from_pycomplex(PyObject *obj) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = COMPLEX128;
    cobj->istype = FALSE;
    cobj->x = PyComplex_RealAsDouble(obj);
    cobj->y = PyComplex_ImagAsDouble(obj);
    
    return cobj;
}

COBJ *ft_create_cobj_from_pyWithUnit(PyObject *obj) {
    PyObject *tobj;
    COBJ *cobj;

    tobj = PyObject_GetAttrString(obj, "_value");
    cobj = ft_create_cobj_from_pyobj(tobj);
    Py_DECREF(tobj);
    if (cobj == NULL) goto exception;

    cobj->str = ft_get_unit_string_ptr(obj);
    // cobj->n = strlen(cobj->str);
    cobj = ft_convert_to_standard_units(cobj);
    if (cobj == NULL) goto exception;
    
    return cobj;
    
exception:
    
    return NULL;
}

char *ft_get_unit_string_ptr(PyObject *obj) {
    int n;
    char *str1, *str2;
    PyObject *tobj1, *tobj2;
    
    tobj1 = PyObject_GetAttrString(obj, "unit");
    tobj2 = PyObject_GetAttrString(tobj1, "name");
    str1 = ((PyStringObject *)tobj2)->ob_sval;
    n = PyString_GET_SIZE(tobj2);
    str2 = (char *)malloc((n+1)*sizeof(char));
    strcpy(str2, str1);
    Py_DECREF(tobj2);
    Py_DECREF(tobj1);
    
    return str2;
}

COBJ *ft_convert_to_standard_units(COBJ *cobj) {
    if (cobj->type == FLOAT64) cobj->type = FLOAT64_SU;
    else if (cobj->type == COMPLEX128) cobj->type = COMPLEX128_SU;
    else if (cobj->type == NDARRAY) cobj->type = NDARRAY_SU;
    else {
        PyErr_SetString(PyExc_TypeError, "unable to conver WithUnit to standard units (SU)");
        ft_free_cobj(cobj);
        return NULL;
    }
    
    return cobj;
}

COBJ *ft_create_cobj_from_pystring(PyObject *obj) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = STRING;
    cobj->istype = FALSE;
    cobj->str = ((PyStringObject *)obj)->ob_sval;
    cobj->n = PyString_GET_SIZE(obj);
    
    return cobj;
}

COBJ *ft_create_cobj_from_pycluster(PyObject *obj) {
    unsigned int ui;
    COBJ *cobj;
    PyObject *tobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = CLUSTER;
    cobj->istype = FALSE;
    cobj->n = PyTuple_GET_SIZE(obj);
    cobj->cobj_arr = (COBJ **)calloc(cobj->n, sizeof(COBJ *));
    
    for (ui=0; ui<cobj->n; ui++) {
        tobj = PyTuple_GET_ITEM(obj, ui);
        cobj->cobj_arr[ui] = ft_create_cobj_from_pyobj(tobj);
        if (cobj->cobj_arr[ui] == NULL) goto exception;
    }
    
    return cobj;
    
exception:
    
    ft_free_cobj(cobj);
    
    return NULL;
}

COBJ *ft_create_cobj_from_pylist(PyObject *obj) {
    unsigned int ui;
    COBJ *cobj;
    PyObject *tobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = LIST;
    cobj->istype = FALSE;
    cobj->n = PyList_GET_SIZE(obj);
    cobj->cobj_arr = (COBJ **)calloc(cobj->n, sizeof(COBJ *));
    
    for (ui=0; ui<cobj->n; ui++) {
        tobj = PyList_GET_ITEM(obj, ui);
        cobj->cobj_arr[ui] = ft_create_cobj_from_pyobj(tobj);
        if (cobj->cobj_arr[ui] == NULL) goto exception;
    }
    
    return cobj;
    
exception:
    
    ft_free_cobj(cobj);
    
    return NULL;
}

COBJ *ft_create_cobj_from_pyndarray(PyObject *obj) {
    unsigned int ui;
    PyObject *tobj;
    PyTypeObject *type;
    PyArrayObject *aobj;
    COBJ *cobj;
    
    tobj = PyObject_GetAttrString(obj, "dtype");
    type = (PyTypeObject *)PyObject_GetAttrString(tobj, "type");
    Py_DECREF(tobj);
    
    aobj = (PyArrayObject *)obj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = NDARRAY;
    cobj->istype = FALSE;
    cobj->n = aobj->nd;
    cobj->dim_arr = (int *)malloc(cobj->n*sizeof(int));
    for (ui=0; ui<cobj->n; ui++) cobj->dim_arr[ui] = aobj->dimensions[ui];
    cobj->isarrowned = FALSE;
    cobj->arr = (void *)aobj->data;
    
    if (type == ft_gv_numpy_bool_) cobj->subtype = BOOL;
    else if (type == ft_gv_numpy_int32) cobj->subtype = INT32;
    else if (type == ft_gv_numpy_uint32) cobj->subtype = UINT32;
    else if (type == ft_gv_numpy_float64) cobj->subtype = FLOAT64;
    else if (type == ft_gv_numpy_complex128) cobj->subtype = COMPLEX128;
    else {
        PyErr_SetString(PyExc_TypeError, "unsupported numpy ndarray dtype");
        goto exception;
    }
    
    return cobj;
    
exception:
    
    ft_free_cobj(cobj);
    
    return NULL;
}

COBJ *ft_create_cobj_from_bool(char b) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = BOOL;
    cobj->istype = FALSE;
    cobj->i = (int)b;
    
    return cobj;
}

COBJ *ft_create_cobj_from_int32(int i) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = INT32;
    cobj->istype = FALSE;
    cobj->i = i;
    
    return cobj;
}

COBJ *ft_create_cobj_from_uint32(unsigned int n) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = UINT32;
    cobj->istype = FALSE;
    cobj->n = n;
    
    return cobj;
}

COBJ *ft_create_cobj_from_float64(double x) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = FLOAT64;
    cobj->istype = FALSE;
    cobj->x = x;
    
    return cobj;
}

COBJ *ft_create_cobj_from_complex128(double x, double y) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = COMPLEX128;
    cobj->istype = FALSE;
    cobj->x = x;
    cobj->y = y;
    
    return cobj;
}

COBJ *ft_create_cobj(int type, int istype) {
    COBJ *cobj;
    
    cobj = ft_allocate_memory_for_cobj();
    
    cobj->type = type;
    cobj->istype = istype;
    
    if (type == LIST) {
        cobj->n = 1;
        cobj->cobj_arr = (COBJ **)malloc(sizeof(COBJ *));
    }
    
    return cobj;
}

void ft_free_cobj(COBJ *cobj) {
    unsigned int ui;
    
    if (cobj == NULL) return;
    
    switch (cobj->type) {
        case FLOAT64_SU:
        case COMPLEX128_SU:
        // case STRING:
        case NDARRAY_SU:
            free(cobj->str);
    }
    
    switch (cobj->type) {
        case CLUSTER:
        case LIST:
            for (ui=0; ui<cobj->n; ui++) {
                ft_free_cobj(cobj->cobj_arr[ui]);
            }
            free(cobj->cobj_arr);
    }
    
    switch (cobj->type) {
        case NDARRAY:
        case NDARRAY_SU:
            free(cobj->dim_arr);
            if (cobj->isarrowned) free(cobj->arr);
    }
    
    free(cobj);
}

COBJ *ft_create_cobj_type(COBJ *cobj) {
    unsigned int ui;
    COBJ *cobj_type;
    
    cobj_type = calloc(1, sizeof(COBJ));
    
    if (cobj == NULL) {
        PyErr_SetString(PyExc_TypeError, "NULL has no type");
        goto exception;
    }
    
    cobj_type->type = cobj->type;
    cobj_type->istype = TRUE;
    
    if (cobj->type == FLOAT64_SU || cobj->type == COMPLEX128_SU || cobj->type == NDARRAY_SU) {
        cobj_type->str = (char *)malloc((strlen(cobj->str)+1)*sizeof(char));
        strcpy(cobj_type->str, cobj->str);
    }
    
    if (cobj->type == NDARRAY || cobj->type == NDARRAY_SU) {
        cobj_type->subtype = cobj->subtype;
        cobj_type->n = cobj->n;
        cobj_type->dim_arr = (int *)malloc(cobj->n*sizeof(int));
        for (ui=0; ui<cobj_type->n; ui++) cobj_type->dim_arr[ui] = cobj->dim_arr[ui];
        cobj_type->isarrowned = FALSE;
        cobj_type->arr = NULL;
        return cobj_type;
    }
    
    if (cobj->type == CLUSTER) {
        cobj_type->n = cobj->n;
        cobj_type->cobj_arr = (COBJ **)malloc(cobj_type->n*sizeof(COBJ *));
        for (ui=0; ui<cobj_type->n; ui++) {
            cobj_type->cobj_arr[ui] = ft_create_cobj_type(cobj->cobj_arr[ui]);
            if (cobj_type->cobj_arr[ui] == NULL) goto exception;
        }
        return cobj_type;
    }
    
    if (cobj->type == LIST) {
        cobj_type->n = 1;
        cobj_type->cobj_arr = (COBJ **)malloc(sizeof(COBJ *));
        cobj_type->cobj_arr[0] = ft_create_list_type(cobj);
        if (cobj_type->cobj_arr[0] == NULL) goto exception;
    }

    return cobj_type;
    
exception:
    
    ft_free_cobj(cobj_type);
    
    return NULL;
}

COBJ *ft_create_list_type(COBJ *cobj) {
    unsigned int ui;
    COBJ *cobj_type1, *cobj_type2;
    
    if (cobj->n == 0) {
        cobj_type1 = calloc(1, sizeof(COBJ));
        cobj_type1->type = EMPTY;
        cobj_type1->istype = TRUE;
        return cobj_type1;
    }
    
    cobj_type1 = ft_create_cobj_type(cobj->cobj_arr[0]);
    if (cobj_type1 == NULL) goto exception;
    
    for (ui=1; ui<cobj->n; ui++) {
        cobj_type2 = ft_create_cobj_type(cobj->cobj_arr[ui]);
        if (cobj_type2 == NULL) {
            ft_free_cobj(cobj_type1);
            goto exception;
        }
        cobj_type1 = ft_upgrade_first_cobj_to_encompass_second(cobj_type1, cobj_type2);
        ft_free_cobj(cobj_type2);
        if (cobj_type1 == NULL) goto exception;
    }
    
    return cobj_type1;
    
exception:
    
    return NULL;
}

/*
COBJ *ft_homogenize_list(COBJ *cobj) {
    unsigned int ui;
    COBJ *cobj_copy;
    
    if (cobj == NULL) return NULL;
    if (cobj->type != LIST && cobj->type != CLUSTER) return cobj;
    if (cobj->n == 0) return cobj;
    
    if (cobj->type == LIST) {
        cobj_copy = ft_create_cobj_type(cobj->cobj_arr[0]);
        
        for (ui=1; ui<cobj->n; ui++) {
            cobj_copy = ft_upgrade_first_cobj_to_encompass_second(cobj_copy, cobj->cobj_arr[ui]);
            if (cobj_copy == NULL) goto exception;
        }
        
        for (ui=0; ui<cobj->n; ui++) {
            cobj->cobj_arr[ui] = ft_upgrade_first_cobj_to_encompass_second(cobj->cobj_arr[ui], cobj_copy);
        }
        
        ft_free_cobj(cobj_copy);
    }
    
    if (cobj->type == CLUSTER) {
        for (ui=0; ui<cobj->n; ui++) {
            cobj->cobj_arr[ui] = ft_homogenize_list(cobj->cobj_arr[ui]);
            if (cobj->cobj_arr[ui] == NULL) goto exception;
        }
    }
    
    return cobj;
    
exception:
    
    ft_free_cobj(cobj);
    
    return NULL;
}
*/

// handles both cobj1->istype == TRUE and FALSE, cobj2->istype == TRUE always
COBJ *ft_upgrade_first_cobj_to_encompass_second(COBJ *cobj1, COBJ *cobj2) {
    unsigned int ui;
    COBJ *cobj_copy;
    /* int N1, N2; */
    
    // handle NULL, EMPTY
    if (cobj1 == NULL || cobj2 == NULL) goto exception;
    
    if (cobj1->type == EMPTY){
        if (cobj1->istype == FALSE) return cobj1;
        if (cobj2->type == EMPTY) return cobj1;
        else {
            ft_free_cobj(cobj1);
            return ft_create_cobj_type(cobj2);
        }
    }
        
    if (cobj2->type == EMPTY) return cobj1;
    
    // handle BOOL, STRING, TIME, ERROR
    if ((cobj1->type == BOOL && cobj2->type != BOOL) ||
        (cobj1->type == STRING && cobj2->type != STRING) ||
        (cobj1->type == TIME && cobj2->type != TIME) ||
        (cobj1->type == ERROR && cobj2->type != ERROR)) {
        goto exception;
    }
    
    if ((cobj1->type == BOOL && cobj2->type == BOOL) ||
        (cobj1->type == STRING && cobj2->type == STRING) ||
        (cobj1->type == TIME && cobj2->type == TIME) ||
        (cobj1->type == ERROR && cobj2->type == ERROR)) {
        return cobj1;
    }
    
    // handle CLUSTER
    if (cobj1->type == CLUSTER) {
        if (cobj2->type != CLUSTER || cobj1->n != cobj2->n) goto exception;
        
        for (ui=0; ui<cobj1->n; ui++) {
            cobj1->cobj_arr[ui] = ft_upgrade_first_cobj_to_encompass_second(cobj1->cobj_arr[ui], cobj2->cobj_arr[ui]);
            if (cobj1->cobj_arr[ui] == NULL) goto exception;
        }
        
        return cobj1;
    }
    
    // handle UINT31, INT32, UINT32, FLOAT64, COMPLEX128
    if (cobj1->type == UINT31) {
        if (cobj2->type == UINT31) {
            return cobj1;
        }
        if (cobj2->type == INT32) {
            cobj1->type = INT32;
            return cobj1;
        }
        if (cobj2->type == UINT32) {
            cobj1->type = UINT32;
            if (cobj1->istype == FALSE) {
                cobj1->n = (unsigned int)cobj1->i;
            }
            return cobj1;
        }
        if (cobj2->type == FLOAT64) {
            cobj1->type = FLOAT64;
            if (cobj1->istype == FALSE) {
                cobj1->x = (double)cobj1->i;
            }
            return cobj1;
        }
        if (cobj2->type == COMPLEX128) {
            cobj1->type = COMPLEX128;
            if (cobj1->istype == FALSE) {
                cobj1->x = (double)cobj1->i;
                cobj1->y = 0;
            }
            return cobj1;
        }
        goto exception;
    }
    
    if (cobj1->type == INT32) {
        if (cobj2->type == UINT31 || cobj2->type == INT32) {
            return cobj1;
        }
        if (cobj2->type == UINT32 || cobj2->type == FLOAT64) {
            cobj1->type = FLOAT64;
            if (cobj1->istype == FALSE) {
                cobj1->x = (double)cobj1->i;
            }
            return cobj1;
        }
        if (cobj2->type == COMPLEX128) {
            cobj1->type = COMPLEX128;
            if (cobj1->istype == FALSE) {
                cobj1->x = (double)cobj1->i;
                cobj1->y = 0;
            }
            return cobj1;
        }
        goto exception;
    }
    
    if (cobj1->type == UINT32) {
        if (cobj2->type == UINT31 || cobj2->type == UINT32) {
            return cobj1;
        }
        if (cobj2->type == INT32 || cobj2->type == FLOAT64) {
            cobj1->type = FLOAT64;
            if (cobj1->istype == FALSE) {
                cobj1->x = (double)cobj1->n;
            }
            return cobj1;
        }
        if (cobj2->type == COMPLEX128) {
            cobj1->type = COMPLEX128;
            if (cobj1->istype == FALSE) {
                cobj1->x = (double)cobj1->n;
                cobj1->y = 0;
            }
            return cobj1;
        }
        goto exception;
    }
    
    if (cobj1->type == FLOAT64) {
        if (cobj2->type == UINT31 || cobj2->type == INT32 || cobj2->type == UINT32 || cobj2->type == FLOAT64) {
            return cobj1;
        }
        if (cobj2->type == COMPLEX128) {
            cobj1->type = COMPLEX128;
            if (cobj1->istype == FALSE) {
                cobj1->y = 0;
            }
            return cobj1;
        }
        if (cobj2->type == FLOAT64_SU && cobj2->str[0] == '*') {
            cobj2->type = FLOAT64;
            free(cobj2->str);
            return cobj1;
        }
        if (cobj2->type == COMPLEX128_SU && cobj2->str[0] == '*') {
            cobj2->type = COMPLEX128;
            free(cobj2->str);
            cobj1->type = COMPLEX128;
            if (cobj1->istype == FALSE) {
                cobj1->y = 0;
            }
            return cobj1;
        }
        goto exception;
    }
    
    if (cobj1->type == COMPLEX128) {
        if (cobj2->type == UINT31 || cobj2->type == INT32 || cobj2->type == UINT32 || cobj2->type == FLOAT64 || cobj2->type == COMPLEX128) {
            return cobj1;
        }
        if (cobj2->type == COMPLEX128_SU && cobj2->str[0] == '*') {
            cobj2->type = COMPLEX128;
            free(cobj2->str);
            return cobj1;
        }
        goto exception;
    }
    
    // handle FLOAT64_SU, COMPLEX128_SU
    if (cobj1->type == FLOAT64_SU) {
        if (cobj2->type == FLOAT64_SU) {
            if (cobj2->str[0] == '*') {
                free(cobj2->str);
                cobj2->str = (char *)malloc((strlen(cobj1->str)+1)*sizeof(char));
                strcpy(cobj2->str, cobj1->str);
            }
            if (strcmp(cobj1->str, cobj2->str) != 0) goto exception;
            return cobj1;
        }
        if (cobj2->type == COMPLEX128_SU) {
            if (cobj2->str[0] == '*') {
                free(cobj2->str);
                cobj2->str = (char *)malloc((strlen(cobj1->str)+1)*sizeof(char));
                strcpy(cobj2->str, cobj1->str);
            }
            if (strcmp(cobj1->str, cobj2->str) != 0) goto exception;
            cobj1->type = COMPLEX128_SU;
            if (cobj1->istype == FALSE) {
                cobj1->y = 0;
            }
            return cobj1;
        }
        goto exception;
    }
    
    if (cobj1->type == COMPLEX128_SU) {
        if (cobj2->type == COMPLEX128_SU) {
            if (cobj2->str[0] == '*') {
                free(cobj2->str);
                cobj2->str = (char *)malloc((strlen(cobj1->str)+1)*sizeof(char));
                strcpy(cobj2->str, cobj1->str);
            }
            if (strcmp(cobj1->str, cobj2->str) != 0) goto exception;
            return cobj1;
        }
        goto exception;
    }
    
    // handle NDARRAY_SU
    if (cobj1->type == NDARRAY_SU) {
        if (cobj2->type == NDARRAY_SU) {
            if (cobj2->str[0] == '*') {
                free(cobj2->str);
                cobj2->str = (char *)malloc((strlen(cobj1->str)+1)*sizeof(char));
                strcpy(cobj2->str, cobj1->str);
            }
            if (strcmp(cobj1->str, cobj2->str) != 0) goto exception;
            if (cobj1->n != cobj2->n) goto exception;
            for (ui=0; ui<cobj1->n; ui++) {
                if (cobj2->dim_arr[ui] == 0) cobj2->dim_arr[ui] = cobj1->dim_arr[ui];
                if (cobj1->dim_arr[ui] != cobj2->dim_arr[ui]) goto exception;
            }
            cobj1 = ft_upgrade_first_subtype_to_encompass_second(cobj1, cobj2);
            return cobj1;
        }
        goto exception;
    }
    
    if (cobj1->type == NDARRAY) {
        if (cobj2->type == NDARRAY_SU && cobj2->str[0] == '*') {
            cobj2->type = NDARRAY;
            free(cobj2->str);
        }
        if (cobj2->type == NDARRAY) {
            if (cobj1->n != cobj2->n) goto exception;
            /*
            N1 = N2 = 1;
            for (ui=0; ui<cobj1->n; ui++) {
                N1 *= cobj1->dim_arr[ui];
                N2 *= cobj2->dim_arr[ui];
            }
            if (N1 != N2) goto exception;
            */
            cobj1 = ft_upgrade_first_subtype_to_encompass_second(cobj1, cobj2);
            if (cobj1 == NULL) return NULL;
            for (ui=0; ui<cobj1->n; ui++) {
                if (cobj2->dim_arr[ui] == 0) cobj2->dim_arr[ui] = cobj1->dim_arr[ui];
                if (cobj1->dim_arr[ui] != cobj2->dim_arr[ui]) {
                    cobj1 = ft_convert_ndarray_to_list(cobj1);
                    break;
                }
            }
            return cobj1;
        }
        if (cobj2->type == LIST) {
            cobj1 = ft_convert_ndarray_to_list(cobj1);
            if (cobj1 == NULL) return NULL;
            return ft_upgrade_first_cobj_to_encompass_second(cobj1, cobj2);
        }
        goto exception;
    }
    
    if (cobj1->type == LIST) {
        if (cobj2->type == NDARRAY) {
            cobj_copy = ft_create_cobj_type(cobj2);
            cobj_copy = ft_convert_ndarray_to_list(cobj_copy);
            if (cobj_copy == NULL) goto exception;
            cobj1 = ft_upgrade_first_cobj_to_encompass_second(cobj1, cobj_copy);
            ft_free_cobj(cobj_copy);
            return cobj1;
        }
        if (cobj2->type == LIST) {
            if (cobj1->n == 0 || cobj2->cobj_arr[0]->type == EMPTY) return cobj1;
            for (ui=0; ui<cobj1->n; ui++) {
                cobj1->cobj_arr[ui] = ft_upgrade_first_cobj_to_encompass_second(cobj1->cobj_arr[ui], cobj2->cobj_arr[0]);
                if (cobj1->cobj_arr[ui] == NULL) goto exception;
            }
            return cobj1;
        }
        goto exception;
    }
    
    goto exception;
    
exception:
    
    PyErr_SetString(PyExc_TypeError, "unable to upgrade one object to encompass another, inhomogeneous list or type tag/object mismatch");
    ft_free_cobj(cobj1);
    
    return NULL;
}

COBJ *ft_upgrade_first_subtype_to_encompass_second(COBJ *cobj1, COBJ *cobj2) {
    unsigned int ui;
    int i, N;
    double *arr;
    
    // handle dimension number disparity
    if (cobj1->n != cobj2->n) {
        PyErr_SetString(PyExc_TypeError, "ndarrays have different number of dimensions");
        goto exception;
    }
    
    if (cobj1->subtype == cobj2->subtype) return cobj1;
    
    // handle BOOL disparity
    if (cobj1->subtype == BOOL || cobj2->subtype == BOOL) {
        PyErr_SetString(PyExc_TypeError, "ndarrays have inconsistent bool/numeric types");
        goto exception;
    }
    
    // upgrade data if required
    N = 1;
    for (ui=0; ui<cobj1->n; ui++) N *= cobj1->dim_arr[ui];
    
    if (cobj1->subtype == INT32 || cobj1->subtype == UINT32) {
        if (cobj2->subtype == INT32 || cobj2->subtype == UINT32 || cobj2->subtype == FLOAT64) {
            cobj1->subtype = FLOAT64;
            if (!cobj1->istype) {
                arr = (double *)malloc(N*LABRAD_FLOAT_SIZE);
                for (i=0; i<N; i++) arr[i] = ((double *)cobj1->arr)[i];
                cobj1->arr = arr;
                cobj1->isarrowned = TRUE;
            }
            return cobj1;
        }
        if (cobj2->subtype == COMPLEX128) {
            cobj1->subtype = COMPLEX128;
            if (!cobj1->istype) {
                arr = (double *)calloc(2*N, LABRAD_FLOAT_SIZE);
                for (i=0; i<2*N; i+=2) arr[i] = ((double *)cobj1->arr)[i];
                cobj1->arr = arr;
                cobj1->isarrowned = TRUE;
            }
            return cobj1;
        }
        PyErr_SetString(PyExc_TypeError, "unsupported ndarray subtype");
        goto exception;
    }
    
    if (cobj1->subtype == FLOAT64) {
        if (cobj2->subtype == INT32 || cobj2->subtype == UINT32 || cobj2->subtype == FLOAT64) return cobj1;
        if (cobj2->subtype == COMPLEX128) {
            cobj1->subtype = COMPLEX128;
            if (!cobj1->istype) {
                arr = (double *)calloc(2*N, LABRAD_FLOAT_SIZE);
                for (i=0; i<2*N; i+=2) arr[i] = ((double *)cobj1->arr)[i];
                cobj1->arr = arr;
                cobj1->isarrowned = TRUE;
            }
            return cobj1;
        }
        PyErr_SetString(PyExc_TypeError, "unsupported ndarray subtype");
        goto exception;
    }
    
    if (cobj1->subtype == COMPLEX128) return cobj1;
    
    PyErr_SetString(PyExc_TypeError, "unsupported ndarray subtype");
    goto exception;
    
exception:
    
    ft_free_cobj(cobj1);
    
    return NULL;
}

COBJ *ft_convert_ndarray_to_list(COBJ *cobj) {
    int i;
    COBJ *tcobj;
    
    if (cobj == NULL) {
        cobj = ft_create_empty_cobj();
        cobj->n = 0;
    }
    
    cobj->type = LIST;
    
    if (cobj->subtype != BOOL && cobj->subtype != INT32 && cobj->subtype != UINT32 && cobj->subtype != FLOAT64 && cobj->subtype != COMPLEX128) goto exception;
    
    if (cobj->n == 0) {
        if (!cobj->istype) {
            cobj->cobj_arr = NULL;
            return cobj;
        }
        cobj->n = 1;
        cobj->cobj_arr = (COBJ **)malloc(sizeof(COBJ *));
        cobj->cobj_arr[0] = calloc(1, sizeof(COBJ));
        cobj->cobj_arr[0]->type = EMPTY;
        cobj->cobj_arr[0]->istype = TRUE;
        return cobj;
    }
    
    
    if (!cobj->istype) {
        cobj->cobj_arr = (COBJ **)malloc(cobj->dim_arr[0]*sizeof(COBJ *));
    }
    else {
        cobj->cobj_arr = (COBJ **)malloc(sizeof(COBJ *));
    }
    
    if (cobj->n == 1) {
        if (!cobj->istype) {
            if (cobj->subtype == BOOL) {
                for (i=0; i<cobj->dim_arr[0]; i++) {
                    cobj->cobj_arr[i] = ft_create_cobj_from_bool(((char *)cobj->arr)[i]);
                }
            }
            else if (cobj->subtype == INT32) {
                for (i=0; i<cobj->dim_arr[0]; i++) {
                    cobj->cobj_arr[i] = ft_create_cobj_from_int32(((int *)cobj->arr)[i]);
                }
            }
            else if (cobj->subtype == UINT32) {
                for (i=0; i<cobj->dim_arr[0]; i++) {
                    cobj->cobj_arr[i] = ft_create_cobj_from_uint32(((unsigned int *)cobj->arr)[i]);
                }
            }
            else if (cobj->subtype == FLOAT64) {
                for (i=0; i<cobj->dim_arr[0]; i++) {
                    cobj->cobj_arr[i] = ft_create_cobj_from_float64(((double *)cobj->arr)[i]);
                }
            }
            else if (cobj->subtype == COMPLEX128) {
                for (i=0; i<cobj->dim_arr[0]; i++) {
                    cobj->cobj_arr[i] = ft_create_cobj_from_complex128(((double *)cobj->arr)[i<<1], ((double *)cobj->arr)[(i<<1)+1]);
                }
            }
            cobj->n = (unsigned int)cobj->dim_arr[0];
            return cobj;
        }
        else {
            cobj->cobj_arr[0] = calloc(1, sizeof(COBJ));
            cobj->cobj_arr[0]->type = cobj->subtype;
            cobj->cobj_arr[0]->istype = TRUE;
            return cobj;
        }
    }
    
    if (!cobj->istype) {
        for (i=0; i<cobj->dim_arr[0]; i++) {
            cobj->cobj_arr[i] = ft_create_cobj_from_ndarray_i(cobj, i);
            cobj->cobj_arr[i] = ft_convert_ndarray_to_list(cobj->cobj_arr[i]);
        }
        cobj->n = (unsigned int)cobj->dim_arr[0];
    }
    else {
        tcobj = cobj;
        while (cobj->n > 0) {
            cobj->n--;
            tcobj->cobj_arr = (COBJ **)malloc(sizeof(COBJ *));
            tcobj->cobj_arr[0] = calloc(1, sizeof(COBJ));
            tcobj = tcobj->cobj_arr[0];
            tcobj->type = LIST;
            tcobj->istype = TRUE;
            tcobj->n = 1;
        }
        cobj->n = 1;
        tcobj->type = cobj->subtype;
    }
    
    return cobj;
    
exception:
    
    PyErr_SetString(PyExc_TypeError, "unsupported ndarray type, cannot convert to list");
    ft_free_cobj(cobj);
    
    return NULL;
}

COBJ *ft_create_cobj_from_ndarray_i(COBJ *cobj, int i) {
    COBJ *cobj_i;
    unsigned int ui;
    int N;
    
    cobj_i = ft_allocate_memory_for_cobj();
    
    cobj_i->type = NDARRAY;
    cobj_i->subtype = cobj->subtype;
    cobj_i->istype = FALSE;
    cobj_i->n = cobj->n - 1;
    cobj_i->dim_arr = (int *)malloc(cobj_i->n*sizeof(int));
    for (ui=0; ui<cobj_i->n; ui++) cobj_i->dim_arr[ui] = cobj->dim_arr[ui+1];
    
    N = 1;
    for (ui=0; ui<cobj_i->n; ui++) N *= cobj_i->dim_arr[ui];
    
    cobj_i->arr = (char *)cobj->arr + i*N*ft_size_subtype(cobj);
    
    return cobj_i;
}

int ft_size(COBJ *cobj) {
    int type, size, N;
    unsigned int ui;
    
    if (cobj == NULL) return 0;
    
    type = cobj->type;
    
    if (type == EMPTY) return 0;
    if (type == BOOL) return LABRAD_BOOL_SIZE;
    if (type == UINT31 || type == INT32 || type == UINT32) return LABRAD_INT_SIZE;
    if (type == FLOAT64 || type == FLOAT64_SU) return LABRAD_FLOAT_SIZE;
    if (type == COMPLEX128 || type == COMPLEX128_SU) return 2*LABRAD_FLOAT_SIZE;
    if (type == STRING) return LABRAD_INT_SIZE + cobj->n;
    if (type == CLUSTER || type == LIST) {
        size = LABRAD_INT_SIZE;
        for (ui=0; ui<cobj->n; ui++) {
            size += ft_size(cobj->cobj_arr[ui]);
        }
        return size;
    }
    if (type == NDARRAY || type == NDARRAY_SU) {
        size = LABRAD_INT_SIZE + cobj->n*LABRAD_INT_SIZE;
        N = 1;
        for (ui=0; ui<cobj->n; ui++) N *= cobj->dim_arr[ui];
        size += N*ft_size_subtype(cobj);
        return size;
    }
    if (type == TIME) return 2*LABRAD_LONG_SIZE;
    if (type == ERROR) {
        size = 2*LABRAD_INT_SIZE;
        size += cobj->n;
        return size;
    }
    
    PyErr_SetString(PyExc_TypeError, "unsupported object type");
    
    return EXCEPTION_RAISED;
}

int ft_size_subtype(COBJ *cobj) {
    int subtype;
    
    subtype = cobj->subtype;
    
    if (subtype == BOOL) return LABRAD_BOOL_SIZE;
    if (subtype == UINT31 || subtype == INT32 || subtype == UINT32) return LABRAD_INT_SIZE;
    if (subtype == FLOAT64) return LABRAD_FLOAT_SIZE;
    if (subtype == COMPLEX128) return 2*LABRAD_FLOAT_SIZE;
    
    PyErr_SetString(PyExc_TypeError, "unsupported ndarray subtype");
    
    return EXCEPTION_RAISED;
}

COBJ *ft_create_cobj_tag_from_pystring(PyObject *tt) {
    int i, n, len;
    char *buf;
    COBJ *cobj_tag, *cobj_cluster;
    
    if (!PyString_Check(tt)) {
        PyErr_SetString(PyExc_TypeError, "type tag must be a string");
        goto exception;
    }
    
    i = 0;
    n = PyString_GET_SIZE(tt);
    buf = ((PyStringObject *)tt)->ob_sval;
    // printf("buf: %s\n", buf);
    
    if (n == 0) {
        PyErr_SetString(PyExc_SyntaxError, "type tag must have non-zero length");
        goto exception;
    }
    
    cobj_tag = ft_create_cobj_tag_from_char_buf(buf, &i, n);
    if (cobj_tag == NULL) goto exception;
    // printf("after ft_create_type_tree_char i: %d, n: %d\n", i, n);
    
    if (i != n) {
        len = 0;
        cobj_cluster = ft_create_cobj(CLUSTER, TRUE);
        cobj_cluster->cobj_arr = (COBJ **)malloc(sizeof(COBJ *));
        cobj_cluster->cobj_arr[len] = cobj_tag;
        cobj_tag = cobj_cluster;
        len = 1;
        while (i < n) {
            cobj_tag->cobj_arr = (COBJ **)realloc(cobj_tag->cobj_arr, (len+1)*sizeof(COBJ *));
            cobj_tag->cobj_arr[len] = ft_create_cobj_tag_from_char_buf(buf, &i, n);
            if (cobj_tag->cobj_arr[len] == NULL) {
                ft_free_cobj(cobj_tag);
                goto exception;
            }
            len++;
        }
        cobj_tag->n = len;
        return cobj_tag;
        
        // PyErr_SetString(PyExc_SyntaxError, "superfluous characters at end of type tag");
        // ft_free_cobj(cobj_tag);
        // goto exception;
    }
    
    return cobj_tag;
    
exception:
    
    return NULL;
}

COBJ *ft_create_cobj_tag_from_char_buf(char *buf, int *iptr, int n) {
    int i, j, len;
    COBJ *cobj_tag, *sub;
    
    i = *iptr;
    
    if (i == n) return NULL;
    
    if (buf[i] == '_' || buf[i] == '?') {
        (*iptr)++;
        return ft_create_cobj(EMPTY, TRUE);
    }
    else if (buf[i] == 'b') {
        (*iptr)++;
        return ft_create_cobj(BOOL, TRUE);
    }
    else if (buf[i] == 'i') {
        (*iptr)++;
        return ft_create_cobj(INT32, TRUE);
    }
    else if (buf[i] == 'w') {
        (*iptr)++;
        return ft_create_cobj(UINT32, TRUE);
    }
    else if (buf[i] == 'v' || buf[i] == 'c') {
        if (buf[i] == 'v') cobj_tag = ft_create_cobj(FLOAT64_SU, TRUE);
        else cobj_tag = ft_create_cobj(COMPLEX128_SU, TRUE);
        if (i == n-1 || buf[i+1] != '[') {
            cobj_tag->str = (char *)malloc(2*sizeof(char));
            strcpy(cobj_tag->str, "*");
            (*iptr)++;
            return cobj_tag;
        }
        i += 2;
        len = 0;
        while (i+len < n) {
            if (buf[i + len] == ']') break;
            len++;
        }
        if (i+len >= n) {
            PyErr_SetString(PyExc_SyntaxError, "missing closing ]");
            ft_free_cobj(cobj_tag);
            goto exception;
        }
        if (len == 0) {
            cobj_tag->str = (char *)malloc(2*sizeof(char));
            strcpy(cobj_tag->str, "*");
            *iptr = i + 1;
            return cobj_tag;
        }
        cobj_tag->str = (char *)malloc((len+1)*sizeof(char));
        strncpy(cobj_tag->str, buf + i, len);
        cobj_tag->str[len] = '\0';
        *iptr = i + len + 1;
        return cobj_tag;
    }
    else if (buf[i] == 's') {
        cobj_tag = ft_create_cobj(STRING, TRUE);
        (*iptr)++;
        return cobj_tag;
    }
    else if (buf[i] == '*') {
        i++;
        if (i == n) {
            PyErr_SetString(PyExc_SyntaxError, "list with no sub type tag");
            goto exception;
        }
        // take care of cases that imply '*' must correspond to a LIST
        switch (buf[i]) {
            case 's':
            case '(':
            case '*':
            case 't':
            case 'E':
            case '_':
            case '?':
                cobj_tag = ft_create_cobj(LIST, TRUE);
                cobj_tag->cobj_arr[0] = ft_create_cobj_tag_from_char_buf(buf, &i, n);
                if (cobj_tag->cobj_arr[0] == NULL) {
                    ft_free_cobj(cobj_tag);
                    goto exception;
                }
                *iptr = i;
                return cobj_tag;
        }
        len = 0;
        while (i+len < n && isdigit(buf[i+len])) len++;
        if (i+len >= n) {
            PyErr_SetString(PyExc_SyntaxError, "ndarray with no sub type tag");
            goto exception;
        }
        if (len > 1) {
            PyErr_SetString(PyExc_SyntaxError, "ndarrays of dimension > 9 not supported");
            goto exception;
        }
        if (buf[i] == '0') {
            PyErr_SetString(PyExc_SyntaxError, "ndarrays of dimension 0 not supported");
            goto exception;
        }
        switch (buf[i+len]) {
            case 's':
            case '(':
            case '*':
            case 't':
            case 'E':
            case '_':
            case '?':
                cobj_tag = sub = ft_create_cobj(LIST, TRUE);
                for (j=1; j<buf[i]-'0'; j++) {
                    sub->cobj_arr[0] = ft_create_cobj(LIST, TRUE);
                    sub = sub->cobj_arr[0];
                }
                i++;
                sub->cobj_arr[0] = ft_create_cobj_tag_from_char_buf(buf, &i, n);
                if (cobj_tag->cobj_arr[0] == NULL) {
                    ft_free_cobj(cobj_tag);
                    goto exception;
                }
                *iptr = i;
                return cobj_tag;
                
                // PyErr_SetString(PyExc_SyntaxError, "unsupported ndarray subtype");
                // goto exception;
        }
        cobj_tag = ft_create_cobj(NDARRAY_SU, TRUE);
        if (len == 0) cobj_tag->n = 1;
        else cobj_tag->n = buf[i] - '0';
        cobj_tag->dim_arr = (int *)calloc(cobj_tag->n, sizeof(int));
        i = i + len;
        sub = ft_create_cobj_tag_from_char_buf(buf, &i, n);
        switch (sub->type) {
            case BOOL:
            case INT32:
            case UINT32:
                cobj_tag->type = NDARRAY;
            case FLOAT64_SU:
            case COMPLEX128_SU:
                cobj_tag->subtype = sub->type;
                break;
            default:
                PyErr_SetString(PyExc_SyntaxError, "unsupported ndarray subtype");
                ft_free_cobj(cobj_tag);
                ft_free_cobj(sub);
                goto exception;
        }
        switch (sub->type) {
            case FLOAT64_SU:
            case COMPLEX128_SU:
                cobj_tag->str = (char *)malloc((strlen(sub->str)+1)*sizeof(char));
                strcpy(cobj_tag->str, sub->str);
        }
        ft_free_cobj(sub);
        *iptr = i;
        return cobj_tag;
    }
    else if (buf[i] == '(') {
        i++;
        if (i == n) {
            PyErr_SetString(PyExc_SyntaxError, "tuple with no )");
            goto exception;
        }
        cobj_tag = ft_create_cobj(CLUSTER, TRUE);
        cobj_tag->cobj_arr = NULL;
        len = 0;
        
        while (i < n && buf[i] != ')') {
            cobj_tag->cobj_arr = (COBJ **)realloc(cobj_tag->cobj_arr, (len+1)*sizeof(COBJ *));
            cobj_tag->cobj_arr[len] = ft_create_cobj_tag_from_char_buf(buf, &i, n);
            if (cobj_tag->cobj_arr[len] == NULL) {
                ft_free_cobj(cobj_tag);
                goto exception;
            }
            len++;
        }
        if (i == n) {
            PyErr_SetString(PyExc_SyntaxError, "tuple with no )");
            goto exception;
        }
        *iptr = i+1;
        cobj_tag->n = len;
        return cobj_tag;
    }
    else {
        PyErr_SetString(PyExc_SyntaxError, "invalid type tag");
        goto exception;
    }
    
exception:
    
    return NULL;
}

int ft_initialize() {
    PyObject *tobj;
    
    if (!ft_system_supported()) {
        PyErr_SetString(PyExc_AssertionError, "system has unsupported data type sizes");
        goto exception;
    }

    ft_detect_system_endianness();
    
    ft_gv_0 = PyInt_FromLong(0);
    ft_gv_1 = PyInt_FromLong(1);
    ft_gv_2 = PyInt_FromLong(2);
    ft_gv_int_num_bits = PyInt_FromLong(8*LABRAD_INT_SIZE);
    ft_gv_int_num_bits_minus_1 = PyInt_FromLong(8*LABRAD_INT_SIZE-1);
    
    tobj = PyNumber_Power(ft_gv_2, ft_gv_int_num_bits, Py_None);
    ft_gv_UINT_MAX = PyNumber_Subtract(tobj, ft_gv_1);
    Py_DECREF(tobj);
    
    tobj = PyNumber_Power(ft_gv_2, ft_gv_int_num_bits_minus_1, Py_None);
    ft_gv_INT_MIN = PyNumber_Negative(tobj);
    Py_DECREF(tobj);
    
    tobj = PyNumber_Power(ft_gv_2, ft_gv_int_num_bits_minus_1, Py_None);
    ft_gv_INT_MAX = PyNumber_Subtract(tobj, ft_gv_1);
    Py_DECREF(tobj);
    
    ft_gv_numpy = PyImport_ImportModule("numpy");
    if (PyErr_Occurred() != NULL) goto exception;
    
    ft_gv_numpy_ndarray = (PyTypeObject *)PyObject_GetAttrString(ft_gv_numpy, "ndarray");
    ft_gv_numpy_bool_ = (PyTypeObject *)PyObject_GetAttrString(ft_gv_numpy, "bool_");
    ft_gv_numpy_int32 = (PyTypeObject *)PyObject_GetAttrString(ft_gv_numpy, "int32");
    ft_gv_numpy_uint32 = (PyTypeObject *)PyObject_GetAttrString(ft_gv_numpy, "uint32");
    ft_gv_numpy_float64 = (PyTypeObject *)PyObject_GetAttrString(ft_gv_numpy, "float64");
    ft_gv_numpy_complex128 = (PyTypeObject *)PyObject_GetAttrString(ft_gv_numpy, "complex128");
    if (PyErr_Occurred() != NULL) goto exception;
    
    ft_gv_labrad_types = PyImport_ImportModule("labrad.types");
    if (PyErr_Occurred() != NULL) goto exception;

    ft_gv_labrad_units = PyImport_ImportModule("labrad.units");
    if (PyErr_Occurred() != NULL) goto exception;

    ft_gv_labrad_units_WithUnit = (PyTypeObject *)PyObject_GetAttrString(ft_gv_labrad_units, "WithUnit");
    if (PyErr_Occurred() != NULL) goto exception;
    
    ft_gv_labrad_units_Value = (PyTypeObject *)PyObject_GetAttrString(ft_gv_labrad_units, "Value");
    if (PyErr_Occurred() != NULL) goto exception;
    
    ft_gv_labrad_units_Complex = (PyTypeObject *)PyObject_GetAttrString(ft_gv_labrad_units, "Complex");
    if (PyErr_Occurred() != NULL) goto exception;
    
    ft_gv_labrad_units_ValueArray = (PyTypeObject *)PyObject_GetAttrString(ft_gv_labrad_units, "ValueArray");
    if (PyErr_Occurred() != NULL) goto exception;
    
    ft_gv_initialized = TRUE;

    return SUCCESS;
    
exception:

    return EXCEPTION_RAISED;
}
 
int ft_system_supported() {
    if (sizeof(char) != LABRAD_BOOL_SIZE ||
        sizeof(int) != LABRAD_INT_SIZE ||
        sizeof(long long int) != LABRAD_LONG_SIZE ||
        sizeof(double) != LABRAD_FLOAT_SIZE)
        return FALSE;
    
    return TRUE;
}

void ft_detect_system_endianness() {
    int i;
    char buf[LABRAD_INT_SIZE];
    
    i = 1;
    memcpy(buf, &i, LABRAD_INT_SIZE);
    
    if (buf[0] == 0) ft_gv_system_big_endian = TRUE;
    else ft_gv_system_big_endian = FALSE;
}

int ft_set_send_endianness(PyObject *endianness) {
    PyObject *big_endian, *little_endian;
    
    big_endian = PyString_FromString(BIG_ENDIAN);
    little_endian = PyString_FromString(LITTLE_ENDIAN);
    
    if (!PyString_Check(endianness)) {
        PyErr_SetString(PyExc_TypeError, "endianness must be '>' or '<'");
        goto exception;
    }
    
    if (PyObject_RichCompareBool(endianness, big_endian, Py_EQ) == TRUE) {
        ft_gv_send_big_endian = TRUE;
    }
    else if (PyObject_RichCompareBool(endianness, little_endian, Py_EQ) == TRUE) {
        ft_gv_send_big_endian = FALSE;
    }
    else {
        PyErr_SetString(PyExc_SyntaxError, "endianness must be '>' or '<'");
        goto exception;
    }
    
    Py_DECREF(big_endian);
    Py_DECREF(little_endian);
    
    return TRUE;
    
exception:
    
    Py_DECREF(big_endian);
    Py_DECREF(little_endian);
    
    return FALSE;
}

void ft_print_cobj(COBJ *cobj) {
    unsigned int ui, N;
    
    if (cobj == NULL) return;
    
    printf("%p\n", cobj);
    ft_print_type(cobj);
    
    if (cobj->istype) printf("istype: TRUE\n");
    else printf("istype: FALSE\n");
    
    switch (cobj->type) {
        case NDARRAY:
        case NDARRAY_SU:
        case ERROR:
            ft_print_subtype(cobj);
    }
    if (!cobj->istype) {
        switch (cobj->type) {
            case BOOL:
                if (cobj->i == FALSE) printf("i: FALSE\n");
                else printf("i: TRUE\n");
                break;
            case UINT31:
            case INT32:
                printf("i: %d\n", cobj->i);
                break;
        }
    }
    switch (cobj->type) {
        case CLUSTER:
        case NDARRAY:
        case NDARRAY_SU:
            printf("n: %u\n", cobj->n);
            break;
        case UINT32:
        case LIST:
        case STRING:
            if (!cobj->istype) {
                printf("n: %u\n", cobj->n);
            }
    }
    if (!cobj->istype) {
        switch (cobj->type) {
            case FLOAT64:
            case FLOAT64_SU:
            case COMPLEX128:
            case COMPLEX128_SU:
                printf("x: %g\n", cobj->x);
        }
        switch (cobj->type) {
            case COMPLEX128:
            case COMPLEX128_SU:
                printf("y: %g\n", cobj->y);
        }
    }
    switch (cobj->type) {
        case FLOAT64_SU:
        case COMPLEX128_SU:
        case NDARRAY_SU:
            printf("str: %s\n", cobj->str);
            break;
        case STRING:
        case ERROR:
            if (!cobj->istype) {
                printf("str: %s\n", cobj->str);
            }
    }
    switch (cobj->type) {
        case CLUSTER:
        case LIST:
            for (ui=0; ui<cobj->n; ui++) {
                printf("cobj_arr[%u]: %p\n", ui, cobj->cobj_arr[ui]);
            }
            for (ui=0; ui<cobj->n; ui++) {
                printf("\n");
                ft_print_cobj(cobj->cobj_arr[ui]);
            }
    }
    switch (cobj->type) {
        case NDARRAY:
        case NDARRAY_SU:
            N = 1;
            for (ui=0; ui<cobj->n; ui++) {
                printf("dim_arr[%u]: %d\n", ui, cobj->dim_arr[ui]);
                N *= cobj->dim_arr[ui];
            }
            if (!cobj->istype) {
                for (ui=0; ui<N; ui++) {
                    if (cobj->subtype == BOOL) {
                        printf("arr[%u]: %d\n", ui, ((char *)cobj->arr)[ui]);
                    }
                    else if (cobj->subtype == UINT31 || cobj->subtype == INT32) {
                        printf("arr[%u]: %d\n", ui, ((int *)cobj->arr)[ui]);
                    }
                    else if (cobj->subtype == UINT32) {
                        printf("arr[%u]: %u\n", ui, ((unsigned int *)cobj->arr)[ui]);
                    }
                    else if (cobj->subtype == FLOAT64) {
                        printf("arr[%u]: %g\n", ui, ((double *)cobj->arr)[ui]);
                    }
                    else if (cobj->subtype == COMPLEX128) {
                        printf("arr[%u]: %g + %gj\n", ui, ((double *)cobj->arr)[2*ui], ((double *)cobj->arr)[2*ui+1]);
                    }
                }
            }
    }
    if (!cobj->istype) {
        switch (cobj->type) {
            case TIME:
                printf("secs: %ld\n", cobj->secs);
                printf("usecs: %ld\n", cobj->usecs);
        }
    }
    
    // printf("\n");
}

void ft_print_type(COBJ *cobj) {
    printf("type: ");
    ft_print_type_text(cobj->type);
    printf("\n");
}

void ft_print_subtype(COBJ *cobj) {
    printf("subtype: ");
    ft_print_type_text(cobj->subtype);
    printf("\n");
}

void ft_print_type_text(int type) {
   switch (type) {
        case EMPTY:
            printf("EMPTY");
            break;
        case BOOL:
            printf("BOOL");
            break;
        case UINT31:
            printf("UINT31");
            break;
        case INT32:
            printf("INT32");
            break;
        case UINT32:
            printf("UINT32");
            break;
        case FLOAT64:
            printf("FLOAT64");
            break;
        case COMPLEX128:
            printf("COMPLEX128");
            break;
        case FLOAT64_SU:
            printf("FLOAT64_SU");
            break;
        case COMPLEX128_SU:
            printf("COMPLEX128_SU");
            break;
        case STRING:
            printf("STRING");
            break;
        case CLUSTER:
            printf("CLUSTER");
            break;
        case LIST:
            printf("LIST");
            break;
        case NDARRAY:
            printf("NDARRAY");
            break;
        case NDARRAY_SU:
            printf("NDARRAY_SU");
            break;
        case TIME:
            printf("TIME");
            break;
        case ERROR:
            printf("ERROR");
            break;
    }
 }

PyObject *ft_test(PyObject *self, PyObject *args) {
    int i, n;
    PyObject *tc_mod, *tc_list, *tc_i;
    COBJ *cobj, *cobj_type;

    if (!ft_gv_initialized) {
        if (ft_initialize() == EXCEPTION_RAISED) goto exception;
    }
    
    tc_mod = PyImport_ImportModule("fasttypes_testcases");
    if (PyErr_Occurred() != NULL) goto exception;
    
    tc_list = PyObject_GetAttrString(tc_mod, "tests");
    if (tc_list == NULL || !PyList_Check(tc_list)) goto exception;
    
    n = PyList_GET_SIZE(tc_list);
    for (i=0; i<n; i++) {
        printf("%d\n", i);
        tc_i = PyList_GET_ITEM(tc_list, i);

        if (tc_i != NULL) {
            PyObject_Print(tc_i, stdout, Py_PRINT_RAW);
            printf("\n\n");
            
            cobj = ft_create_cobj_from_pyobj(tc_i);
            if (cobj == NULL) goto exception;
            ft_print_cobj(cobj);
            printf("\n");
            
            cobj_type = ft_create_cobj_type(cobj);
            if (cobj_type == NULL) {
                ft_free_cobj(cobj);
                goto exception;
            }
            ft_print_cobj(cobj_type);
            printf("\n");
            
            ft_free_cobj(cobj);
            ft_free_cobj(cobj_type);
        }
    }
    
    printf("All %d test cases passed.\n", n);
    
    Py_DECREF(tc_list);
    Py_DECREF(tc_mod);
    
    Py_INCREF(Py_None);
    return Py_None;
    
exception:

    if (tc_list != NULL) Py_DECREF(tc_list);
    if (tc_mod != NULL) Py_DECREF(tc_mod);

    return NULL;
}

static PyMethodDef fasttypes_methods[] = {
    {"flatten", (PyCFunction)ft_flatten, METH_VARARGS | METH_KEYWORDS, "flatten() doc string."},
    {"test", ft_test, METH_VARARGS, "test() doc string."},
    {NULL, NULL}
};

PyMODINIT_FUNC initfasttypes(void) {
    Py_InitModule("fasttypes", fasttypes_methods);
}