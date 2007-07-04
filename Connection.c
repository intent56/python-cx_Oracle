//-----------------------------------------------------------------------------
// Connection.c
//   Definition of the Python type OracleConnection.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// structure for the Python type "Connection"
//-----------------------------------------------------------------------------
typedef struct {
    PyObject_HEAD
    OCISvcCtx *handle;
    OCIServer *serverHandle;
    OCISession *sessionHandle;
    udt_Environment *environment;
#ifdef ORACLE_9I
    udt_SessionPool *sessionPool;
#endif
    PyObject *username;
    PyObject *password;
    PyObject *dsn;
    PyObject *version;
    ub4 commitMode;
} udt_Connection;


//-----------------------------------------------------------------------------
// constants for the OCI attributes
//-----------------------------------------------------------------------------
#ifdef ORACLE_10G
static ub4 gc_ModuleAttribute = OCI_ATTR_MODULE;
static ub4 gc_ActionAttribute = OCI_ATTR_ACTION;
static ub4 gc_ClientInfoAttribute = OCI_ATTR_CLIENT_INFO;
#endif


//-----------------------------------------------------------------------------
// functions for the Python type "Connection"
//-----------------------------------------------------------------------------
static void Connection_Free(udt_Connection*);
static PyObject *Connection_New(PyTypeObject*, PyObject*, PyObject*);
static int Connection_Init(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_Repr(udt_Connection*);
static PyObject *Connection_Close(udt_Connection*, PyObject*);
static PyObject *Connection_Commit(udt_Connection*, PyObject*);
static PyObject *Connection_Begin(udt_Connection*, PyObject*);
static PyObject *Connection_Prepare(udt_Connection*, PyObject*);
static PyObject *Connection_Rollback(udt_Connection*, PyObject*);
static PyObject *Connection_NewCursor(udt_Connection*, PyObject*);
static PyObject *Connection_Cancel(udt_Connection*, PyObject*);
static PyObject *Connection_RegisterCallback(udt_Connection*, PyObject*);
static PyObject *Connection_UnregisterCallback(udt_Connection*, PyObject*);
static PyObject *Connection_GetVersion(udt_Connection*, void*);
static PyObject *Connection_GetMaxBytesPerCharacter(udt_Connection*, void*);
static PyObject *Connection_ContextManagerEnter(udt_Connection*, PyObject*);
static PyObject *Connection_ContextManagerExit(udt_Connection*, PyObject*);
#ifdef OCI_NLS_CHARSET_MAXBYTESZ
static PyObject *Connection_GetEncoding(udt_Connection*, void*);
static PyObject *Connection_GetNationalEncoding(udt_Connection*, void*);
#endif
#ifdef ORACLE_10G
static int Connection_SetOCIAttr(udt_Connection*, PyObject*, ub4*);
#endif


//-----------------------------------------------------------------------------
// declaration of methods for Python type "Connection"
//-----------------------------------------------------------------------------
static PyMethodDef g_ConnectionMethods[] = {
    { "cursor", (PyCFunction) Connection_NewCursor, METH_NOARGS },
    { "commit", (PyCFunction) Connection_Commit, METH_NOARGS },
    { "rollback", (PyCFunction) Connection_Rollback, METH_NOARGS },
    { "begin", (PyCFunction) Connection_Begin, METH_VARARGS },
    { "prepare", (PyCFunction) Connection_Prepare, METH_NOARGS },
    { "close", (PyCFunction) Connection_Close, METH_NOARGS },
    { "cancel", (PyCFunction) Connection_Cancel, METH_NOARGS },
    { "register", (PyCFunction) Connection_RegisterCallback, METH_VARARGS },
    { "unregister", (PyCFunction) Connection_UnregisterCallback, METH_VARARGS },
    { "__enter__", (PyCFunction) Connection_ContextManagerEnter, METH_NOARGS },
    { "__exit__", (PyCFunction) Connection_ContextManagerExit, METH_VARARGS },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for Python type "Connection"
//-----------------------------------------------------------------------------
static PyMemberDef g_ConnectionMembers[] = {
    { "username", T_OBJECT, offsetof(udt_Connection, username), READONLY },
    { "password", T_OBJECT, offsetof(udt_Connection, password), READONLY },
    { "dsn", T_OBJECT, offsetof(udt_Connection, dsn), READONLY },
    { "tnsentry", T_OBJECT, offsetof(udt_Connection, dsn), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of calculated members for Python type "Connection"
//-----------------------------------------------------------------------------
static PyGetSetDef g_ConnectionCalcMembers[] = {
#ifdef OCI_NLS_CHARSET_MAXBYTESZ
    { "encoding", (getter) Connection_GetEncoding, 0, 0, 0 },
    { "nencoding", (getter) Connection_GetNationalEncoding, 0, 0, 0 },
#endif
    { "version", (getter) Connection_GetVersion, 0, 0, 0 },
    { "maxBytesPerCharacter", (getter) Connection_GetMaxBytesPerCharacter,
            0, 0, 0 },
#ifdef ORACLE_10G
    { "module", 0, (setter) Connection_SetOCIAttr, 0, &gc_ModuleAttribute },
    { "action", 0, (setter) Connection_SetOCIAttr, 0, &gc_ActionAttribute },
    { "clientinfo", 0, (setter) Connection_SetOCIAttr, 0,
            &gc_ClientInfoAttribute },
#endif
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type "Connection"
//-----------------------------------------------------------------------------
static PyTypeObject g_ConnectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "cx_Oracle.Connection",             // tp_name
    sizeof(udt_Connection),             // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Connection_Free,       // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) Connection_Repr,         // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                        // tp_flags
    0,                                  // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    g_ConnectionMethods,                // tp_methods
    g_ConnectionMembers,                // tp_members
    g_ConnectionCalcMembers,            // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc) Connection_Init,         // tp_init
    0,                                  // tp_alloc
    (newfunc) Connection_New,           // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};


//-----------------------------------------------------------------------------
// Connection_IsConnected()
//   Determines if the connection object is connected to the database. If not,
// a Python exception is raised.
//-----------------------------------------------------------------------------
static int Connection_IsConnected(
    udt_Connection *self)               // connection to check
{
    if (!self->handle) {
        PyErr_SetString(g_InterfaceErrorException, "not connected");
        return -1;
    }
    return 0;
}


#ifdef ORACLE_9I
//-----------------------------------------------------------------------------
// Connection_Acquire()
//   Acquire a connection from a session pool.
//-----------------------------------------------------------------------------
static int Connection_Acquire(
    udt_Connection *self,               // connection
    udt_SessionPool *pool)              // pool to acquire connection from
{
    boolean found;
    sword status;

    // acquire the session from the pool
    Py_BEGIN_ALLOW_THREADS
    status = OCISessionGet(pool->environment->handle,
            pool->environment->errorHandle, &self->handle, NULL,
            (OraText*) PyString_AS_STRING(pool->name),
            PyString_GET_SIZE(pool->name), NULL, 0, NULL, NULL, &found,
            OCI_SESSGET_SPOOL);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(pool->environment, status,
            "Connection_Acquire()") < 0)
        return -1;

    // initialize members
    Py_INCREF(pool);
    self->sessionPool = pool;
    Py_INCREF(pool->username);
    self->username = pool->username;
    Py_INCREF(pool->password);
    self->password = pool->password;
    Py_INCREF(pool->dsn);
    self->dsn = pool->dsn;

    return 0;
}
#endif


#ifdef ORACLE_10G
//-----------------------------------------------------------------------------
// Connection_SetOCIAttr()
//   Set the value of the OCI attribute.
//-----------------------------------------------------------------------------
static int Connection_SetOCIAttr(
    udt_Connection *self,               // connection to set
    PyObject *value,                    // value to set
    ub4 *attribute)                     // OCI attribute type
{
    sword status;

    // make sure connection is connected
    if (Connection_IsConnected(self) < 0)
        return -1;

    // set the value in the OCI
    if (!PyString_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "value must be a string");
        return -1;
    }
    status = OCIAttrSet(self->sessionHandle, OCI_HTYPE_SESSION,
            PyString_AS_STRING(value), PyString_GET_SIZE(value),
            *attribute, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_SetOCIAttr()") < 0)
        return -1;
    return 0;
}
#endif


//-----------------------------------------------------------------------------
// Connection_Attach()
//   Attach to an existing connection.
//-----------------------------------------------------------------------------
static int Connection_Attach(
    udt_Connection *self,               // connection
    OCISvcCtx *handle)                  // handle of connection to attach to
{
    OCISession *sessionHandle;
    OCIServer *serverHandle;
    sword status;

    // acquire the server handle
    status = OCIAttrGet(handle, OCI_HTYPE_SVCCTX, (dvoid**) &serverHandle, 0,
            OCI_ATTR_SERVER, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Attach(): determine server handle") < 0)
        return -1;

    // acquire the session handle
    status = OCIAttrGet(handle, OCI_HTYPE_SVCCTX, (dvoid**) &sessionHandle, 0,
            OCI_ATTR_SESSION, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Attach(): determine session handle") < 0)
        return -1;

    // allocate the service context handle
    status = OCIHandleAlloc(self->environment->handle,
            (dvoid*) &self->handle, OCI_HTYPE_SVCCTX, 0, 0);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Attach(): allocate service context handle") < 0)
        return -1;

    // set attribute for server handle
    status = OCIAttrSet(self->handle, OCI_HTYPE_SVCCTX, serverHandle, 0,
            OCI_ATTR_SERVER, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Attach(): set server handle") < 0)
        return -1;

    // set attribute for session handle
    status = OCIAttrSet(self->handle, OCI_HTYPE_SVCCTX, sessionHandle, 0,
            OCI_ATTR_SESSION, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Attach(): set session handle") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Connection_Connect()
//   Create a new connection object by connecting to the database.
//-----------------------------------------------------------------------------
static int Connection_Connect(
    udt_Connection *self,               // connection
    const char *dsn,                    // TNS entry
    unsigned dsnLength,                 // length of TNS entry
    ub4 mode,                           // mode to connect as
    int threaded,                       // threaded?
    int twophase)                       // allow two phase commit?
{
    ub4 credentialType = OCI_CRED_EXT;
    sword status;

    // allocate the server handle
    status = OCIHandleAlloc(self->environment->handle,
            (dvoid**) &self->serverHandle, OCI_HTYPE_SERVER, 0, 0);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): allocate server handle") < 0)
        return -1;

    // attach to the server
    Py_BEGIN_ALLOW_THREADS
    status = OCIServerAttach(self->serverHandle,
            self->environment->errorHandle, (text*) dsn, dsnLength,
            OCI_DEFAULT);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): server attach") < 0)
        return -1;

    // allocate the service context handle
    status = OCIHandleAlloc(self->environment->handle,
            (dvoid**) &self->handle, OCI_HTYPE_SVCCTX, 0, 0);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): allocate service context handle") < 0)
        return -1;

    // set attribute for server handle
    status = OCIAttrSet(self->handle, OCI_HTYPE_SVCCTX, self->serverHandle, 0,
            OCI_ATTR_SERVER, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): set server handle") < 0)
        return -1;

    // set the internal and external names; these are needed for global
    // transactions but are limited in terms of the lengths of the strings
    if (twophase) {
        status = OCIAttrSet(self->serverHandle, OCI_HTYPE_SERVER,
                (dvoid*) "cx_Oracle", 0, OCI_ATTR_INTERNAL_NAME,
                self->environment->errorHandle);
        if (Environment_CheckForError(self->environment, status,
                "Connection_Connect(): set internal name") < 0)
            return -1;
        status = OCIAttrSet(self->serverHandle, OCI_HTYPE_SERVER,
                (dvoid*) "cx_Oracle", 0, OCI_ATTR_EXTERNAL_NAME,
                self->environment->errorHandle);
        if (Environment_CheckForError(self->environment, status,
                "Connection_Connect(): set external name") < 0)
            return -1;
    }

    // allocate the session handle
    status = OCIHandleAlloc(self->environment->handle,
            (dvoid**) &self->sessionHandle, OCI_HTYPE_SESSION, 0, 0);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): allocate session handle") < 0)
        return -1;

    // set user name in session handle
    if (self->username && PyString_GET_SIZE(self->username) > 0) {
        credentialType = OCI_CRED_RDBMS;
        status = OCIAttrSet(self->sessionHandle, OCI_HTYPE_SESSION,
                (text*) PyString_AS_STRING(self->username),
                PyString_GET_SIZE(self->username), OCI_ATTR_USERNAME,
                self->environment->errorHandle);
        if (Environment_CheckForError(self->environment, status,
                "Connection_Connect(): set user name") < 0)
            return -1;
    }

    // set password in session handle
    if (self->password && PyString_GET_SIZE(self->password) > 0) {
        credentialType = OCI_CRED_RDBMS;
        status = OCIAttrSet(self->sessionHandle, OCI_HTYPE_SESSION,
                (text*) PyString_AS_STRING(self->password),
                PyString_GET_SIZE(self->password), OCI_ATTR_PASSWORD,
                self->environment->errorHandle);
        if (Environment_CheckForError(self->environment, status,
                "Connection_Connect(): set password") < 0)
            return -1;
    }

    // set the session handle on the service context handle
    status = OCIAttrSet(self->handle, OCI_HTYPE_SVCCTX,
            self->sessionHandle, 0, OCI_ATTR_SESSION,
            self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): set session handle") < 0)
        return -1;

    // begin the session
    Py_BEGIN_ALLOW_THREADS
    status = OCISessionBegin(self->handle, self->environment->errorHandle,
            self->sessionHandle, credentialType, mode);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Connect(): begin session") < 0) {
        self->sessionHandle = NULL;
        return -1;
    }

    return 0;
}


#include "Cursor.c"
#include "Callback.c"


//-----------------------------------------------------------------------------
// Connection_New()
//   Create a new connection object and return it.
//-----------------------------------------------------------------------------
static PyObject* Connection_New(
    PyTypeObject *type,                 // type object
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    udt_Connection *self;

    // create the object
    self = (udt_Connection*) type->tp_alloc(type, 0);
    if (!self)
        return NULL;
    self->commitMode = OCI_DEFAULT;
    self->environment = NULL;

    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// Connection_Init()
//   Initialize the connection members.
//-----------------------------------------------------------------------------
static int Connection_Init(
    udt_Connection *self,               // connection
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    unsigned usernameLength, passwordLength, dsnLength;
    const char *username, *password, *dsn;
    PyObject *threadedObj, *twophaseObj;
    int threaded, twophase;
#ifdef ORACLE_9I
    udt_SessionPool *pool;
#endif
    OCISvcCtx *handle;
    ub4 connectMode;

    // define keyword arguments
    static char *keywordList[] = { "user", "password", "dsn", "mode", "handle",
#ifdef ORACLE_9I
            "pool",
#endif
            "threaded", "twophase", NULL };

    // parse arguments
    handle = NULL;
    username = password = dsn = NULL;
    threadedObj = twophaseObj = NULL;
    usernameLength = passwordLength = dsnLength = 0;
    threaded = twophase = 0;
    connectMode = OCI_DEFAULT;
#ifdef ORACLE_9I
    pool = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|s#s#s#iiO!OO",
#else
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|s#s#s#iiOO",
#endif
            keywordList, &username, &usernameLength, &password,
            &passwordLength, &dsn, &dsnLength, &connectMode, &handle,
#ifdef ORACLE_9I
            &g_SessionPoolType, &pool,
#endif
            &threadedObj, &twophaseObj))
        return -1;
    if (threadedObj) {
        threaded = PyObject_IsTrue(threadedObj);
        if (threaded < 0)
            return -1;
    }
    if (twophaseObj) {
        twophase = PyObject_IsTrue(twophaseObj);
        if (twophase < 0)
            return -1;
    }

    // set up the environment
    self->environment = Environment_New(threaded);
    if (!self->environment)
        return -1;

    // perform some parsing, if necessary
    if (username) {
        if (!password) {
            password = strchr(username, '/');
            dsn = strchr(username, '@');
            if (password) {
                if (dsn && password > dsn)
                    password = NULL;
                else {
                    password++;
                    passwordLength = strlen(password);
                    usernameLength -= passwordLength + 1;
                }
            }
            if (dsn) {
                if (dsn < password)
                    dsn = NULL;
                else {
                    dsn++;
                    dsnLength = strlen(dsn);
                    if (password)
                        passwordLength -= dsnLength + 1;
                    else usernameLength -= dsnLength + 1;
                }
            }
        }
    }

    // create the string for the username
    if (username) {
        self->username = PyString_FromStringAndSize(username, usernameLength);
        if (!self->username)
            return -1;
    }

    // create the string for the password
    if (password) {
        self->password = PyString_FromStringAndSize(password, passwordLength);
        if (!self->password)
            return -1;
    }

    // create the string for the TNS entry
    if (dsn) {
        self->dsn = PyString_FromStringAndSize(dsn, dsnLength);
        if (!self->dsn)
            return -1;
    }

    // handle the different ways of initializing the connection
    if (handle)
        return Connection_Attach(self, handle);
#ifdef ORACLE_9I
    if (pool)
        return Connection_Acquire(self, pool);
#endif
    return Connection_Connect(self, dsn, dsnLength, connectMode, threaded,
            twophase);
}


//-----------------------------------------------------------------------------
// Connection_Free()
//   Deallocate the connection, disconnecting from the database if necessary.
//-----------------------------------------------------------------------------
static void Connection_Free(
    udt_Connection *self)               // connection object
{
#ifdef ORACLE_9I
    if (self->sessionPool) {
        Py_BEGIN_ALLOW_THREADS
        OCITransRollback(self->handle, self->environment->errorHandle,
                OCI_DEFAULT);
        OCISessionRelease(self->handle, self->environment->errorHandle, NULL,
                0, OCI_DEFAULT);
        Py_END_ALLOW_THREADS
        Py_DECREF(self->sessionPool);
    }
#endif
    if (self->sessionHandle) {
        Py_BEGIN_ALLOW_THREADS
        OCITransRollback(self->handle, self->environment->errorHandle,
                OCI_DEFAULT);
        OCISessionEnd(self->handle, self->environment->errorHandle,
                self->sessionHandle, OCI_DEFAULT);
        Py_END_ALLOW_THREADS
    }
    if (self->serverHandle)
        OCIServerDetach(self->serverHandle,
                self->environment->errorHandle, OCI_DEFAULT);
    Py_XDECREF(self->environment);
    Py_XDECREF(self->username);
    Py_XDECREF(self->password);
    Py_XDECREF(self->dsn);
    Py_XDECREF(self->version);
    self->ob_type->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Connection_Repr()
//   Return a string representation of the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Repr(
    udt_Connection *connection)         // connection to return the string for
{
    PyObject *module, *name, *result;

    if (GetModuleAndName(connection->ob_type, &module, &name) < 0)
        return NULL;
    if (connection->username && connection->username != Py_None &&
            connection->dsn && connection->dsn != Py_None)
        result = PyString_FromFormat("<%s.%s to %s@%s>",
                PyString_AS_STRING(module), PyString_AS_STRING(name),
                PyString_AS_STRING(connection->username),
                PyString_AS_STRING(connection->dsn));
    else if (connection->username && connection->username != Py_None)
        result = PyString_FromFormat("<%s.%s to user %s@local>",
                PyString_AS_STRING(module), PyString_AS_STRING(name),
                PyString_AS_STRING(connection->username));
    else result = PyString_FromFormat("<%s.%s to externally identified user>",
                PyString_AS_STRING(module), PyString_AS_STRING(name));
    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}


#ifdef OCI_NLS_CHARSET_MAXBYTESZ
//-----------------------------------------------------------------------------
// Connection_GetCharacterSetName()
//   Retrieve the IANA character set name for the attribute.
//-----------------------------------------------------------------------------
static PyObject *Connection_GetCharacterSetName(
    udt_Connection *self,               // connection object
    ub2 attribute)                      // attribute to fetch
{
    char charsetName[OCI_NLS_MAXBUFSZ], ianaCharsetName[OCI_NLS_MAXBUFSZ];
    ub2 charsetId;
    sword status;

    // get character set id
    status = OCIAttrGet(self->environment->handle, OCI_HTYPE_ENV, &charsetId,
            NULL, attribute, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_GetCharacterSetName(): get character set id") < 0)
        return NULL;

    // get character set name
    status = OCINlsCharSetIdToName(self->environment->handle,
            (text*) charsetName, OCI_NLS_MAXBUFSZ, charsetId);
    if (Environment_CheckForError(self->environment, status,
            "Connection_GetNEncoding(): get Oracle character set name") < 0)
        return NULL;

    // get IANA character set name
    status = OCINlsNameMap(self->environment->handle,
            (oratext*) ianaCharsetName, OCI_NLS_MAXBUFSZ,
            (oratext*) charsetName, OCI_NLS_CS_ORA_TO_IANA);
    if (Environment_CheckForError(self->environment, status,
            "Connection_GetEncoding(): translate NLS character set") < 0)
        return NULL;

    return PyString_FromString(ianaCharsetName);
}


//-----------------------------------------------------------------------------
// Connection_GetEncoding()
//   Retrieve the IANA encoding used by the client.
//-----------------------------------------------------------------------------
static PyObject *Connection_GetEncoding(
    udt_Connection *self,               // connection object
    void *arg)                          // optional argument (ignored)
{
    return Connection_GetCharacterSetName(self, OCI_ATTR_ENV_CHARSET_ID);
}


//-----------------------------------------------------------------------------
// Connection_GetNationalEncoding()
//   Retrieve the IANA national encoding used by the client.
//-----------------------------------------------------------------------------
static PyObject *Connection_GetNationalEncoding(
    udt_Connection *self,               // connection object
    void *arg)                          // optional argument (ignored)
{
    return Connection_GetCharacterSetName(self, OCI_ATTR_ENV_NCHARSET_ID);
}
#endif


//-----------------------------------------------------------------------------
// Connection_GetVersion()
//   Retrieve the version of the database and return it. Note that this
// function also places the result in the associated dictionary so it is only
// calculated once.
//-----------------------------------------------------------------------------
static PyObject *Connection_GetVersion(
    udt_Connection *self,               // connection object
    void *arg)                          // optional argument (ignored)
{
    udt_Variable *versionVar, *compatVar;
    PyObject *results, *temp;
    udt_Cursor *cursor;

    // if version has already been determined, no need to determine again
    if (self->version) {
        Py_INCREF(self->version);
        return self->version;
    }

    // allocate a cursor to retrieve the version
    cursor = (udt_Cursor*) Connection_NewCursor(self, NULL);
    if (!cursor)
        return NULL;

    // allocate version variable
    versionVar = Variable_New(cursor, cursor->arraySize, &vt_String,
            vt_String.elementLength);
    if (!versionVar) {
        Py_DECREF(cursor);
        return NULL;
    }

    // allocate compatibility variable
    compatVar = Variable_New(cursor, cursor->arraySize, &vt_String,
            vt_String.elementLength);
    if (!compatVar) {
        Py_DECREF(versionVar);
        Py_DECREF(cursor);
        return NULL;
    }

    // create the parameters for the function call
    temp = Py_BuildValue("(s,[OO])",
            "begin dbms_utility.db_version(:ver, :compat); end;",
            versionVar, compatVar);
    Py_DECREF(versionVar);
    Py_DECREF(compatVar);
    if (!temp) {
        Py_DECREF(cursor);
        return NULL;
    }

    // execute the cursor
    results = Cursor_Execute(cursor, temp, NULL);
    if (!results) {
        Py_DECREF(temp);
        Py_DECREF(cursor);
        return NULL;
    }
    Py_DECREF(results);

    // retrieve value
    self->version = Variable_GetValue(versionVar, 0);
    Py_DECREF(temp);
    Py_DECREF(cursor);
    Py_XINCREF(self->version);
    return self->version;
}


//-----------------------------------------------------------------------------
// Connection_GetMaxBytesPerCharacter()
//   Return the maximum number of bytes per character.
//-----------------------------------------------------------------------------
static PyObject *Connection_GetMaxBytesPerCharacter(
    udt_Connection *self,               // connection object
    void *arg)                          // optional argument (ignored)
{
    return PyInt_FromLong(self->environment->maxBytesPerCharacter);
}


//-----------------------------------------------------------------------------
// Connection_Close()
//   Close the connection, disconnecting from the database.
//-----------------------------------------------------------------------------
static PyObject *Connection_Close(
    udt_Connection *self,               // connection to close
    PyObject *args)                     // arguments
{
    sword status;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform a rollback
    Py_BEGIN_ALLOW_THREADS
    status = OCITransRollback(self->handle, self->environment->errorHandle,
            OCI_DEFAULT);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Close(): rollback") < 0)
        return NULL;

    // logoff of the server
    if (self->sessionHandle) {
        Py_BEGIN_ALLOW_THREADS
        status = OCISessionEnd(self->handle, self->environment->errorHandle,
                self->sessionHandle, OCI_DEFAULT);
        Py_END_ALLOW_THREADS
        if (Environment_CheckForError(self->environment, status,
                "Connection_Close(): end session") < 0)
            return NULL;
        OCIHandleFree(self->handle, OCI_HTYPE_SVCCTX);
    }
    self->handle = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_Commit()
//   Commit the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Commit(
    udt_Connection *self,               // connection to commit
    PyObject *args)                     // arguments
{
    sword status;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform the commit
    Py_BEGIN_ALLOW_THREADS
    status = OCITransCommit(self->handle, self->environment->errorHandle,
            self->commitMode);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Commit()") < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_Begin()
//   Begin a new transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Begin(
    udt_Connection *self,               // connection to commit
    PyObject *args)                     // arguments
{
    unsigned transactionIdLength, branchIdLength;
    const char *transactionId, *branchId;
    OCITrans *transactionHandle;
    int formatId;
    sword status;
    XID xid;

    // parse the arguments
    formatId = -1;
    transactionIdLength = branchIdLength = 0;
    if (!PyArg_ParseTuple(args, "|is#s#", &formatId, &transactionId,
            &transactionIdLength,  &branchId, &branchIdLength))
        return NULL;
    if (transactionIdLength > MAXGTRIDSIZE) {
        PyErr_SetString(PyExc_ValueError, "transaction id too large");
        return NULL;
    }
    if (branchIdLength > MAXBQUALSIZE) {
        PyErr_SetString(PyExc_ValueError, "branch id too large");
        return NULL;
    }

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // determine if a transaction handle was previously allocated
    status = OCIAttrGet(self->handle, OCI_HTYPE_SVCCTX,
            (dvoid**) &transactionHandle, 0, OCI_ATTR_TRANS,
            self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Begin(): find existing transaction handle") < 0)
        return NULL;

    // create a new transaction handle, if necessary
    if (!transactionHandle) {
        status = OCIHandleAlloc(self->environment->handle,
                (dvoid**) &transactionHandle, OCI_HTYPE_TRANS, 0, 0);
        if (Environment_CheckForError(self->environment, status,
                "Connection_Begin(): allocate transaction handle") < 0)
            return NULL;
    }

    // set the XID for the transaction, if applicable
    if (formatId != -1) {
        xid.formatID = formatId;
        xid.gtrid_length = transactionIdLength;
        xid.bqual_length = branchIdLength;
        if (transactionIdLength > 0)
            strncpy(xid.data, transactionId, transactionIdLength);
        if (branchIdLength > 0)
            strncpy(&xid.data[transactionIdLength], branchId, branchIdLength);
        OCIAttrSet(transactionHandle, OCI_HTYPE_TRANS, &xid, sizeof(XID),
                OCI_ATTR_XID, self->environment->errorHandle);
        if (Environment_CheckForError(self->environment, status,
                "Connection_Begin(): set XID") < 0)
            return NULL;
    }

    // associate the transaction with the connection
    OCIAttrSet(self->handle, OCI_HTYPE_SVCCTX, transactionHandle, 0,
            OCI_ATTR_TRANS, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Begin(): associate transaction") < 0)
        return NULL;

    // start the transaction
    Py_BEGIN_ALLOW_THREADS
    status = OCITransStart(self->handle, self->environment->errorHandle, 0,
            OCI_TRANS_NEW);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Begin(): start transaction") < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_Prepare()
//   Commit the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Prepare(
    udt_Connection *self,               // connection to commit
    PyObject *args)                     // arguments
{
    sword status;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform the prepare
    Py_BEGIN_ALLOW_THREADS
    status = OCITransPrepare(self->handle, self->environment->errorHandle,
            OCI_DEFAULT);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Prepare()") < 0)
        return NULL;
    self->commitMode = OCI_TRANS_TWOPHASE;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_Rollback()
//   Rollback the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Rollback(
    udt_Connection *self,               // connection to rollback
    PyObject *args)                     // arguments
{
    sword status;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform the rollback
    Py_BEGIN_ALLOW_THREADS
    status = OCITransRollback(self->handle, self->environment->errorHandle,
            OCI_DEFAULT);
    Py_END_ALLOW_THREADS
    if (Environment_CheckForError(self->environment, status,
            "Connection_Rollback()") < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_NewCursor()
//   Create a new cursor (statement) referencing the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_NewCursor(
    udt_Connection *self,               // connection to create cursor on
    PyObject *args)                     // arguments
{
    PyObject *createArgs, *result;

    createArgs = PyTuple_New(1);
    if (!createArgs)
        return NULL;
    Py_INCREF(self);
    PyTuple_SET_ITEM(createArgs, 0, (PyObject*) self);
    result = PyObject_Call( (PyObject*) &g_CursorType, createArgs, NULL);
    Py_DECREF(createArgs);
    return result;
}


//-----------------------------------------------------------------------------
// Connection_Cancel()
//   Execute an OCIBreak() to cause an immediate (asynchronous) abort of any
// currently executing OCI function.
//-----------------------------------------------------------------------------
static PyObject *Connection_Cancel(
    udt_Connection *self,               // connection to cancel
    PyObject *args)                     // arguments
{
    sword status;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform the break
    status = OCIBreak(self->handle, self->environment->errorHandle);
    if (Environment_CheckForError(self->environment, status,
            "Connection_Cancel()") < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_RegisterCallback()
//   Register a callback for the OCI function.
//-----------------------------------------------------------------------------
static PyObject *Connection_RegisterCallback(
    udt_Connection *self,               // connection to register callback on
    PyObject *args)                     // arguments
{
    PyObject *callback, *tuple;
    int functionCode, when;
    sword status;

    // parse the arguments
    if (!PyArg_ParseTuple(args, "iiO", &functionCode, &when, &callback))
        return NULL;

    // create a tuple for passing through to the callback handler
    tuple = Py_BuildValue("OO", self, callback);
    if (!tuple)
        return NULL;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // register the callback with the OCI
    status = OCIUserCallbackRegister(self->environment->handle, OCI_HTYPE_ENV,
            self->environment->errorHandle, (OCIUserCallback) Callback_Handler,
            tuple, functionCode, when, NULL);
    if (Environment_CheckForError(self->environment, status,
            "Connection_RegisterCallback()") < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

//-----------------------------------------------------------------------------
// Connection_UnregisterCallback()
//   Unregister a callback for the OCI function, if one has been registered.
// No error is raised if a callback has not been registered.
//-----------------------------------------------------------------------------
static PyObject *Connection_UnregisterCallback(
    udt_Connection *self,               // connection to unregister callback on
    PyObject *args)                     // arguments
{
    OCIUserCallback callback;
    int functionCode, when;
    PyObject *tuple;
    sword status;

    // parse the arguments
    if (!PyArg_ParseTuple(args, "ii", &functionCode, &when))
        return NULL;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // find out if a callback has been registered
    status = OCIUserCallbackGet(self->environment->handle, OCI_HTYPE_ENV,
            self->environment->errorHandle, functionCode, when, &callback,
            (dvoid**) &tuple, NULL);
    if (Environment_CheckForError(self->environment, status,
            "Connection_UnregisterCallback(): get") < 0)
        return NULL;

    // if a callback was registered, clear it
    if (callback) {
        Py_DECREF(tuple);
        status = OCIUserCallbackRegister(self->environment->handle,
                OCI_HTYPE_ENV, self->environment->errorHandle, NULL,
                NULL, functionCode, when, NULL);
        if (Environment_CheckForError(self->environment, status,
                "Connection_UnregisterCallback(): clear") < 0)
            return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_ContextManagerEnter()
//   Called when the connection is used as a context manager and simply returns
// itself as a convenience to the caller.
//-----------------------------------------------------------------------------
static PyObject *Connection_ContextManagerEnter(
    udt_Connection *self,               // connection
    PyObject* args)                     // arguments
{
    Py_INCREF(self);
    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// Connection_ContextManagerExit()
//   Called when the connection is used as a context manager and if any
// exception a rollback takes place; otherwise, a commit takes place.
//-----------------------------------------------------------------------------
static PyObject *Connection_ContextManagerExit(
    udt_Connection *self,               // connection
    PyObject* args)                     // arguments
{
    PyObject *excType, *excValue, *excTraceback, *result;
    char *methodName;

    if (!PyArg_ParseTuple(args, "OOO", &excType, &excValue, &excTraceback))
        return NULL;
    if (excType == Py_None && excValue == Py_None && excTraceback == Py_None)
        methodName = "commit";
    else methodName = "rollback";
    result = PyObject_CallMethod((PyObject*) self, methodName, "");
    if (!result)
        return NULL;
    Py_DECREF(result);

    Py_INCREF(Py_False);
    return Py_False;
}
