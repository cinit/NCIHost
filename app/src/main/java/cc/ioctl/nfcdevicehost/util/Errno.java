package cc.ioctl.nfcdevicehost.util;

import androidx.annotation.Nullable;

import java.util.HashMap;

/**
 * Linux errno constants.
 */
public class Errno {
    private Errno() {
        throw new AssertionError("No instances.");
    }

    public static final int EPERM = 1; /* Operation not permitted */
    public static final int ENOENT = 2; /* No such file or directory */
    public static final int ESRCH = 3; /* No such process */
    public static final int EINTR = 4; /* Interrupted system call */
    public static final int EIO = 5; /* I/O error */
    public static final int ENXIO = 6; /* No such device or address */
    public static final int E2BIG = 7; /* Argument list too long */
    public static final int ENOEXEC = 8; /* Exec format error */
    public static final int EBADF = 9; /* Bad file number */
    public static final int ECHILD = 10; /* No child processes */
    public static final int EAGAIN = 11; /* Try again */
    public static final int ENOMEM = 12; /* Out of memory */
    public static final int EACCES = 13; /* Permission denied */
    public static final int EFAULT = 14; /* Bad address */
    public static final int ENOTBLK = 15; /* Block device required */
    public static final int EBUSY = 16; /* Device or resource busy */
    public static final int EEXIST = 17; /* File exists */
    public static final int EXDEV = 18; /* Cross-device link */
    public static final int ENODEV = 19; /* No such device */
    public static final int ENOTDIR = 20; /* Not a directory */
    public static final int EISDIR = 21; /* Is a directory */
    public static final int EINVAL = 22; /* Invalid argument */
    public static final int ENFILE = 23; /* File table overflow */
    public static final int EMFILE = 24; /* Too many open files */
    public static final int ENOTTY = 25; /* Not a typewriter */
    public static final int ETXTBSY = 26; /* Text file busy */
    public static final int EFBIG = 27; /* File too large */
    public static final int ENOSPC = 28; /* No space left on device */
    public static final int ESPIPE = 29; /* Illegal seek */
    public static final int EROFS = 30; /* Read-only file system */
    public static final int EMLINK = 31; /* Too many links */
    public static final int EPIPE = 32; /* Broken pipe */
    public static final int EDOM = 33; /* Math argument out of domain of func */
    public static final int ERANGE = 34; /* Math result not representable */
    public static final int EDEADLK = 35; /* Resource deadlock would occur */
    public static final int ENAMETOOLONG = 36; /* File name too long */
    public static final int ENOLCK = 37; /* No record locks available */
    public static final int ENOSYS = 38; /* Invalid system call number */
    public static final int ENOTEMPTY = 39; /* Directory not empty */
    public static final int ELOOP = 40; /* Too many symbolic links encountered */
    public static final int EWOULDBLOCK = EAGAIN; /* Operation would block */
    public static final int ENOMSG = 42; /* No message of desired type */
    public static final int EIDRM = 43; /* Identifier removed */
    public static final int ECHRNG = 44; /* Channel number out of range */
    public static final int EL2NSYNC = 45; /* Level 2 not synchronized */
    public static final int EL3HLT = 46; /* Level 3 halted */
    public static final int EL3RST = 47; /* Level 3 reset */
    public static final int ELNRNG = 48; /* Link number out of range */
    public static final int EUNATCH = 49; /* Protocol driver not attached */
    public static final int ENOCSI = 50; /* No CSI structure available */
    public static final int EL2HLT = 51; /* Level 2 halted */
    public static final int EBADE = 52; /* Invalid exchange */
    public static final int EBADR = 53; /* Invalid request descriptor */
    public static final int EXFULL = 54; /* Exchange full */
    public static final int ENOANO = 55; /* No anode */
    public static final int EBADRQC = 56; /* Invalid request code */
    public static final int EBADSLT = 57; /* Invalid slot */
    public static final int EDEADLOCK = EDEADLK;
    public static final int EBFONT = 59; /* Bad font file format */
    public static final int ENOSTR = 60; /* Device not a stream */
    public static final int ENODATA = 61; /* No data available */
    public static final int ETIME = 62; /* Timer expired */
    public static final int ENOSR = 63; /* Out of streams resources */
    public static final int ENONET = 64; /* Machine is not on the network */
    public static final int ENOPKG = 65; /* Package not installed */
    public static final int EREMOTE = 66; /* Object is remote */
    public static final int ENOLINK = 67; /* Link has been severed */
    public static final int EADV = 68; /* Advertise error */
    public static final int ESRMNT = 69; /* Srmount error */
    public static final int ECOMM = 70; /* Communication error on send */
    public static final int EPROTO = 71; /* Protocol error */
    public static final int EMULTIHOP = 72; /* Multihop attempted */
    public static final int EDOTDOT = 73; /* RFS specific error */
    public static final int EBADMSG = 74; /* Not a data message */
    public static final int EOVERFLOW = 75; /* Value too large for defined data type */
    public static final int ENOTUNIQ = 76; /* Name not unique on network */
    public static final int EBADFD = 77; /* File descriptor in bad state */
    public static final int EREMCHG = 78; /* Remote address changed */
    public static final int ELIBACC = 79; /* Can not access a needed shared library */
    public static final int ELIBBAD = 80; /* Accessing a corrupted shared library */
    public static final int ELIBSCN = 81; /* .lib section in a.out corrupted */
    public static final int ELIBMAX = 82; /* Attempting to link in too many shared libraries */
    public static final int ELIBEXEC = 83; /* Cannot exec a shared library directly */
    public static final int EILSEQ = 84; /* Illegal byte sequence */
    public static final int ERESTART = 85; /* Interrupted system call should be restarted */
    public static final int ESTRPIPE = 86; /* Streams pipe error */
    public static final int EUSERS = 87; /* Too many users */
    public static final int ENOTSOCK = 88; /* Socket operation on non-socket */
    public static final int EDESTADDRREQ = 89; /* Destination address required */
    public static final int EMSGSIZE = 90; /* Message too long */
    public static final int EPROTOTYPE = 91; /* Protocol wrong type for socket */
    public static final int ENOPROTOOPT = 92; /* Protocol not available */
    public static final int EPROTONOSUPPORT = 93; /* Protocol not supported */
    public static final int ESOCKTNOSUPPORT = 94; /* Socket type not supported */
    public static final int EOPNOTSUPP = 95; /* Operation not supported on transport endpoint */
    public static final int EPFNOSUPPORT = 96; /* Protocol family not supported */
    public static final int EAFNOSUPPORT = 97; /* Address family not supported by protocol */
    public static final int EADDRINUSE = 98; /* Address already in use */
    public static final int EADDRNOTAVAIL = 99; /* Cannot assign requested address */
    public static final int ENETDOWN = 100; /* Network is down */
    public static final int ENETUNREACH = 101; /* Network is unreachable */
    public static final int ENETRESET = 102; /* Network dropped connection because of reset */
    public static final int ECONNABORTED = 103; /* Software caused connection abort */
    public static final int ECONNRESET = 104; /* Connection reset by peer */
    public static final int ENOBUFS = 105; /* No buffer space available */
    public static final int EISCONN = 106; /* Transport endpoint is already connected */
    public static final int ENOTCONN = 107; /* Transport endpoint is not connected */
    public static final int ESHUTDOWN = 108; /* Cannot send after transport endpoint shutdown */
    public static final int ETOOMANYREFS = 109; /* Too many references: cannot splice */
    public static final int ETIMEDOUT = 110; /* Connection timed out */
    public static final int ECONNREFUSED = 111; /* Connection refused */
    public static final int EHOSTDOWN = 112; /* Host is down */
    public static final int EHOSTUNREACH = 113; /* No route to host */
    public static final int EALREADY = 114; /* Operation already in progress */
    public static final int EINPROGRESS = 115; /* Operation now in progress */
    public static final int ESTALE = 116; /* Stale file handle */
    public static final int EUCLEAN = 117; /* Structure needs cleaning */
    public static final int ENOTNAM = 118; /* Not a XENIX named type file */
    public static final int ENAVAIL = 119; /* No XENIX semaphores available */
    public static final int EISNAM = 120; /* Is a named type file */
    public static final int EREMOTEIO = 121; /* Remote I/O error */
    public static final int EDQUOT = 122; /* Quota exceeded */
    public static final int ENOMEDIUM = 123; /* No medium found */
    public static final int EMEDIUMTYPE = 124; /* Wrong medium type */
    public static final int ECANCELED = 125; /* Operation Canceled */
    public static final int ENOKEY = 126; /* Required key not available */
    public static final int EKEYEXPIRED = 127; /* Key has expired */
    public static final int EKEYREVOKED = 128; /* Key has been revoked */
    public static final int EKEYREJECTED = 129; /* Key was rejected by service */
    public static final int EOWNERDEAD = 130; /* Owner died */
    public static final int ENOTRECOVERABLE = 131; /* State not recoverable */
    public static final int ERFKILL = 132; /* Operation not possible due to RF-kill */
    public static final int EHWPOISON = 133; /* Memory page has hardware error */

    private static final HashMap<Integer, String[]> ERRNO_VALUES = new HashMap<>(135);

    @Nullable
    public static String getErrnoName(int errno) {
        String[] names = ERRNO_VALUES.get(errno);
        if (names == null) {
            return null;
        }
        return names[0];
    }

    @Nullable
    public static String getErrnoDescription(int errno) {
        String[] names = ERRNO_VALUES.get(errno);
        if (names == null) {
            return null;
        }
        return names[1];
    }

    static {
        ERRNO_VALUES.put(EPERM, new String[]{"EPERM", "Operation not permitted"});
        ERRNO_VALUES.put(ENOENT, new String[]{"ENOENT", "No such file or directory"});
        ERRNO_VALUES.put(ESRCH, new String[]{"ESRCH", "No such process"});
        ERRNO_VALUES.put(EINTR, new String[]{"EINTR", "Interrupted system call"});
        ERRNO_VALUES.put(EIO, new String[]{"EIO", "I/O error"});
        ERRNO_VALUES.put(ENXIO, new String[]{"ENXIO", "No such device or address"});
        ERRNO_VALUES.put(E2BIG, new String[]{"E2BIG", "Argument list too long"});
        ERRNO_VALUES.put(ENOEXEC, new String[]{"ENOEXEC", "Exec format error"});
        ERRNO_VALUES.put(EBADF, new String[]{"EBADF", "Bad file number"});
        ERRNO_VALUES.put(ECHILD, new String[]{"ECHILD", "No child processes"});
        ERRNO_VALUES.put(EAGAIN, new String[]{"EAGAIN", "Resource temporarily unavailable"});
        ERRNO_VALUES.put(ENOMEM, new String[]{"ENOMEM", "Cannot allocate memory"});
        ERRNO_VALUES.put(EACCES, new String[]{"EACCES", "Permission denied"});
        ERRNO_VALUES.put(EFAULT, new String[]{"EFAULT", "Bad address"});
        ERRNO_VALUES.put(ENOTBLK, new String[]{"ENOTBLK", "Block device required"});
        ERRNO_VALUES.put(EBUSY, new String[]{"EBUSY", "Device or resource busy"});
        ERRNO_VALUES.put(EEXIST, new String[]{"EEXIST", "File exists"});
        ERRNO_VALUES.put(EXDEV, new String[]{"EXDEV", "Cross-device link"});
        ERRNO_VALUES.put(ENODEV, new String[]{"ENODEV", "No such device"});
        ERRNO_VALUES.put(ENOTDIR, new String[]{"ENOTDIR", "Not a directory"});
        ERRNO_VALUES.put(EISDIR, new String[]{"EISDIR", "Is a directory"});
        ERRNO_VALUES.put(EINVAL, new String[]{"EINVAL", "Invalid argument"});
        ERRNO_VALUES.put(ENFILE, new String[]{"ENFILE", "Too many open files in system"});
        ERRNO_VALUES.put(EMFILE, new String[]{"EMFILE", "Too many open files"});
        ERRNO_VALUES.put(ENOTTY, new String[]{"ENOTTY", "Not a typewriter"});
        ERRNO_VALUES.put(ETXTBSY, new String[]{"ETXTBSY", "Text file busy"});
        ERRNO_VALUES.put(EFBIG, new String[]{"EFBIG", "File too large"});
        ERRNO_VALUES.put(ENOSPC, new String[]{"ENOSPC", "No space left on device"});
        ERRNO_VALUES.put(ESPIPE, new String[]{"ESPIPE", "Illegal seek"});
        ERRNO_VALUES.put(EROFS, new String[]{"EROFS", "Read-only file system"});
        ERRNO_VALUES.put(EMLINK, new String[]{"EMLINK", "Too many links"});
        ERRNO_VALUES.put(EPIPE, new String[]{"EPIPE", "Broken pipe"});
        ERRNO_VALUES.put(EDOM, new String[]{"EDOM", "Numerical argument out of domain"});
        ERRNO_VALUES.put(ERANGE, new String[]{"ERANGE", "Numerical result out of range"});
        ERRNO_VALUES.put(EDEADLK, new String[]{"EDEADLK", "Resource deadlock would occur"});
        ERRNO_VALUES.put(ENAMETOOLONG, new String[]{"ENAMETOOLONG", "File name too long"});
        ERRNO_VALUES.put(ENOLCK, new String[]{"ENOLCK", "No record locks available"});
        ERRNO_VALUES.put(ENOSYS, new String[]{"ENOSYS", "Function not implemented"});
        ERRNO_VALUES.put(ENOTEMPTY, new String[]{"ENOTEMPTY", "Directory not empty"});
        ERRNO_VALUES.put(ELOOP, new String[]{"ELOOP", "Too many symbolic links encountered"});
        ERRNO_VALUES.put(ENOMSG, new String[]{"ENOMSG", "No message of desired type"});
        ERRNO_VALUES.put(EIDRM, new String[]{"EIDRM", "Identifier removed"});
        ERRNO_VALUES.put(ECHRNG, new String[]{"ECHRNG", "Channel number out of range"});
        ERRNO_VALUES.put(EL2NSYNC, new String[]{"EL2NSYNC", "Level 2 not synchronized"});
        ERRNO_VALUES.put(EL3HLT, new String[]{"EL3HLT", "Level 3 halted"});
        ERRNO_VALUES.put(EL3RST, new String[]{"EL3RST", "Level 3 reset"});
        ERRNO_VALUES.put(ELNRNG, new String[]{"ELNRNG", "Link number out of range"});
        ERRNO_VALUES.put(EUNATCH, new String[]{"EUNATCH", "Protocol driver not attached"});
        ERRNO_VALUES.put(ENOCSI, new String[]{"ENOCSI", "No CSI structure available"});
        ERRNO_VALUES.put(EL2HLT, new String[]{"EL2HLT", "Level 2 halted"});
        ERRNO_VALUES.put(EBADE, new String[]{"EBADE", "Invalid exchange"});
        ERRNO_VALUES.put(EBADR, new String[]{"EBADR", "Invalid request descriptor"});
        ERRNO_VALUES.put(EXFULL, new String[]{"EXFULL", "Exchange full"});
        ERRNO_VALUES.put(ENOANO, new String[]{"ENOANO", "No anode"});
        ERRNO_VALUES.put(EBADRQC, new String[]{"EBADRQC", "Invalid request code"});
        ERRNO_VALUES.put(EBADSLT, new String[]{"EBADSLT", "Invalid slot"});
        ERRNO_VALUES.put(EBFONT, new String[]{"EBFONT", "Bad font file format"});
        ERRNO_VALUES.put(ENOSTR, new String[]{"ENOSTR", "Device not a stream"});
        ERRNO_VALUES.put(ENODATA, new String[]{"ENODATA", "No data available"});
        ERRNO_VALUES.put(ETIME, new String[]{"ETIME", "Timer expired"});
        ERRNO_VALUES.put(ENOSR, new String[]{"ENOSR", "Out of streams resources"});
        ERRNO_VALUES.put(ENONET, new String[]{"ENONET", "Machine is not on the network"});
        ERRNO_VALUES.put(ENOPKG, new String[]{"ENOPKG", "Package not installed"});
        ERRNO_VALUES.put(EREMOTE, new String[]{"EREMOTE", "Object is remote"});
        ERRNO_VALUES.put(ENOLINK, new String[]{"ENOLINK", "Link has been severed"});
        ERRNO_VALUES.put(EADV, new String[]{"EADV", "Advertise error"});
        ERRNO_VALUES.put(ESRMNT, new String[]{"ESRMNT", "Srmount error"});
        ERRNO_VALUES.put(ECOMM, new String[]{"ECOMM", "Communication error on send"});
        ERRNO_VALUES.put(EPROTO, new String[]{"EPROTO", "Protocol error"});
        ERRNO_VALUES.put(EMULTIHOP, new String[]{"EMULTIHOP", "Multihop attempted"});
        ERRNO_VALUES.put(EDOTDOT, new String[]{"EDOTDOT", "RFS specific error"});
        ERRNO_VALUES.put(EBADMSG, new String[]{"EBADMSG", "Not a data message"});
        ERRNO_VALUES.put(EOVERFLOW, new String[]{"EOVERFLOW", "Value too large for defined data type"});
        ERRNO_VALUES.put(ENOTUNIQ, new String[]{"ENOTUNIQ", "Name not unique on network"});
        ERRNO_VALUES.put(EBADFD, new String[]{"EBADFD", "File descriptor in bad state"});
        ERRNO_VALUES.put(EREMCHG, new String[]{"EREMCHG", "Remote address changed"});
        ERRNO_VALUES.put(ELIBACC, new String[]{"ELIBACC", "Can not access a needed shared library"});
        ERRNO_VALUES.put(ELIBBAD, new String[]{"ELIBBAD", "Accessing a corrupted shared library"});
        ERRNO_VALUES.put(ELIBSCN, new String[]{"ELIBSCN", ".lib section in a.out corrupted"});
        ERRNO_VALUES.put(ELIBMAX, new String[]{"ELIBMAX", "Attempting to link in too many shared libraries"});
        ERRNO_VALUES.put(ELIBEXEC, new String[]{"ELIBEXEC", "Cannot exec a shared library directly"});
        ERRNO_VALUES.put(EILSEQ, new String[]{"EILSEQ", "Illegal byte sequence"});
        ERRNO_VALUES.put(ERESTART, new String[]{"ERESTART", "Interrupted system call should be restarted"});
        ERRNO_VALUES.put(ESTRPIPE, new String[]{"ESTRPIPE", "Streams pipe error"});
        ERRNO_VALUES.put(EUSERS, new String[]{"EUSERS", "Too many users"});
        ERRNO_VALUES.put(ENOTSOCK, new String[]{"ENOTSOCK", "Socket operation on non-socket"});
        ERRNO_VALUES.put(EDESTADDRREQ, new String[]{"EDESTADDRREQ", "Destination address required"});
        ERRNO_VALUES.put(EMSGSIZE, new String[]{"EMSGSIZE", "Message too long"});
        ERRNO_VALUES.put(EPROTOTYPE, new String[]{"EPROTOTYPE", "Protocol wrong type for socket"});
        ERRNO_VALUES.put(ENOPROTOOPT, new String[]{"ENOPROTOOPT", "Protocol not available"});
        ERRNO_VALUES.put(EPROTONOSUPPORT, new String[]{"EPROTONOSUPPORT", "Protocol not supported"});
        ERRNO_VALUES.put(ESOCKTNOSUPPORT, new String[]{"ESOCKTNOSUPPORT", "Socket type not supported"});
        ERRNO_VALUES.put(EOPNOTSUPP, new String[]{"EOPNOTSUPP", "Operation not supported on transport endpoint"});
        ERRNO_VALUES.put(EPFNOSUPPORT, new String[]{"EPFNOSUPPORT", "Protocol family not supported"});
        ERRNO_VALUES.put(EAFNOSUPPORT, new String[]{"EAFNOSUPPORT", "Address family not supported by protocol"});
        ERRNO_VALUES.put(EADDRINUSE, new String[]{"EADDRINUSE", "Address already in use"});
        ERRNO_VALUES.put(EADDRNOTAVAIL, new String[]{"EADDRNOTAVAIL", "Cannot assign requested address"});
        ERRNO_VALUES.put(ENETDOWN, new String[]{"ENETDOWN", "Network is down"});
        ERRNO_VALUES.put(ENETUNREACH, new String[]{"ENETUNREACH", "Network is unreachable"});
        ERRNO_VALUES.put(ENETRESET, new String[]{"ENETRESET", "Network dropped connection because of reset"});
        ERRNO_VALUES.put(ECONNABORTED, new String[]{"ECONNABORTED", "Software caused connection abort"});
        ERRNO_VALUES.put(ECONNRESET, new String[]{"ECONNRESET", "Connection reset by peer"});
        ERRNO_VALUES.put(ENOBUFS, new String[]{"ENOBUFS", "No buffer space available"});
        ERRNO_VALUES.put(EISCONN, new String[]{"EISCONN", "Transport endpoint is already connected"});
        ERRNO_VALUES.put(ENOTCONN, new String[]{"ENOTCONN", "Transport endpoint is not connected"});
        ERRNO_VALUES.put(ESHUTDOWN, new String[]{"ESHUTDOWN", "Cannot send after transport endpoint shutdown"});
        ERRNO_VALUES.put(ETOOMANYREFS, new String[]{"ETOOMANYREFS", "Too many references: cannot splice"});
        ERRNO_VALUES.put(ETIMEDOUT, new String[]{"ETIMEDOUT", "Connection timed out"});
        ERRNO_VALUES.put(ECONNREFUSED, new String[]{"ECONNREFUSED", "Connection refused"});
        ERRNO_VALUES.put(EHOSTDOWN, new String[]{"EHOSTDOWN", "Host is down"});
        ERRNO_VALUES.put(EHOSTUNREACH, new String[]{"EHOSTUNREACH", "No route to host"});
        ERRNO_VALUES.put(EALREADY, new String[]{"EALREADY", "Operation already in progress"});
        ERRNO_VALUES.put(EINPROGRESS, new String[]{"EINPROGRESS", "Operation now in progress"});
        ERRNO_VALUES.put(ESTALE, new String[]{"ESTALE", "Stale NFS file handle"});
        ERRNO_VALUES.put(EUCLEAN, new String[]{"EUCLEAN", "Structure needs cleaning"});
        ERRNO_VALUES.put(ENOTNAM, new String[]{"ENOTNAM", "Not a XENIX named type file"});
        ERRNO_VALUES.put(ENAVAIL, new String[]{"ENAVAIL", "No XENIX semaphores available"});
        ERRNO_VALUES.put(EISNAM, new String[]{"EISNAM", "Is a named type file"});
        ERRNO_VALUES.put(EREMOTEIO, new String[]{"EREMOTEIO", "Remote I/O error"});
        ERRNO_VALUES.put(EDQUOT, new String[]{"EDQUOT", "Quota exceeded"});
        ERRNO_VALUES.put(ENOMEDIUM, new String[]{"ENOMEDIUM", "No medium found"});
        ERRNO_VALUES.put(EMEDIUMTYPE, new String[]{"EMEDIUMTYPE", "Wrong medium type"});
        ERRNO_VALUES.put(ECANCELED, new String[]{"ECANCELED", "Operation Canceled"});
        ERRNO_VALUES.put(ENOKEY, new String[]{"ENOKEY", "Required key not available"});
        ERRNO_VALUES.put(EKEYEXPIRED, new String[]{"EKEYEXPIRED", "Key has expired"});
        ERRNO_VALUES.put(EKEYREVOKED, new String[]{"EKEYREVOKED", "Key has been revoked"});
        ERRNO_VALUES.put(EKEYREJECTED, new String[]{"EKEYREJECTED", "Key was rejected by service"});
        ERRNO_VALUES.put(EOWNERDEAD, new String[]{"EOWNERDEAD", "Owner died"});
        ERRNO_VALUES.put(ENOTRECOVERABLE, new String[]{"ENOTRECOVERABLE", "State not recoverable"});
        ERRNO_VALUES.put(ERFKILL, new String[]{"ERFKILL", "Operation not possible due to RF-kill"});
        ERRNO_VALUES.put(EHWPOISON, new String[]{"EHWPOISON", "Memory page has hardware error"});
    }

    @Nullable
    public static String getErrnoString(int errno) {
        errno = Math.abs(errno);
        String[] err = ERRNO_VALUES.get(errno);
        if (err != null) {
            return err[0] + "(" + errno + "): " + err[1];
        } else {
            return null;
        }
    }
}
