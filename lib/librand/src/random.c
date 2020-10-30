#include "random.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/random.h>


static int urandomOpen(void);
static void urandomClose(int fd);

static int setFlags(void);
static int checkIntegrity(int fd);

static size_t readRandomBytes(void *dest, size_t n, int fd);
static int checkEntropy(int fd, size_t n);


/* Fill a buffer with random bytes from /dev/urandom */
int getRandomBytes(void *dest, size_t n)
{
    int fd;

    if (!dest)
        return 1;

    fd = urandomOpen();

    if (fd < 0)
        return 1;

    if (readRandomBytes(dest, n, fd) < n)
    {
        urandomClose(fd);
        return 1;
    }

    urandomClose(fd);

    return 0;
}


/* Get a uniformly distributed long int between min and max (inclusive)
 * Note: the range cannot be LONG_MIN to LONG_MAX
 */
int getRandomULong(unsigned long int *x, unsigned long int min, unsigned long int max)
{
    int fd;
    unsigned long int random;
    unsigned long int range = max - min + 1UL;

    if (!x || max < min)
        return 1;
    else if (max == ULONG_MAX && min == 0)
        return -1;

    fd = urandomOpen();

    if (fd < 0)
        return 1;
    
    /* Say the random number, r, is an unsigned 4-bit type, ranging from 0 to
     * 15 and is to be limited to 0 to 9. A simple r % 10 will result in the
     * numbers 0 to 5 appearing more than 6 to 9.
     * Hence, numbers generated >= 10 (15 - (15 % 10)) are discarded. This
     * ensures a uniform distribution of numbers from 0 to 10.
     * The range can then be shifted by a minimum to get into the desired range.
     */
    do
    {
        if (readRandomBytes(&random, sizeof(random), fd) < sizeof(random))
        {
            urandomClose(fd);
            return 1;
        }
    }
    while (random >= ULONG_MAX - (ULONG_MAX % range));
    
    urandomClose(fd);

    *x = min + (random % range);

    return 0;
}


/* Get a uniformly distributed long int between min and max (inclusive)
 * Note: the range cannot be LONG_MIN to LONG_MAX
 */
int getRandomLong(long int *x, long int min, long int max)
{
    int fd;
    unsigned long int randomUL;
    unsigned long int range = (unsigned long int) (max - min) + 1UL;

    if (!x || max < min)
        return 1;
    else if (max == LONG_MAX && min == LONG_MIN)
        return -1;

    fd = urandomOpen();

    if (fd < 0)
        return 1;

    do
    {
        if (readRandomBytes(&randomUL, sizeof(randomUL), fd) < sizeof(randomUL))
        {
            urandomClose(fd);
            return 1;
        }
    }
    while (randomUL >= ULONG_MAX - (ULONG_MAX % range));
    
    urandomClose(fd);

    *x = min + (long int) (randomUL % range);

    return 0;
}


static int urandomOpen(void)
{
    const char *URANDOM_PATH = "/dev/urandom";

    /* Set open flags and open urandom */
    int flags = setFlags();
    int fd = open(URANDOM_PATH, flags);

    if (fd == -1)
        return -1;

    /* If CLOEXEC flag could not be set by setFlag(), set with fcntl() */
    #ifndef O_CLOEXEC
    if (fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC) == -1)
    {
        urandomClose(fd);
        return -1;
    }
    #endif

    /* Ensure file descriptor does actually point to urandom */
    if (checkIntegrity(fd))
    {
        urandomClose(fd);
        return -1;
    }

    return fd;
}


static void urandomClose(int fd)
{
    close(fd);
}


static int setFlags(void)
{
    /* Set file for reading only */
    int flags = O_RDONLY;

    /* If file is a symbolic link, open() fails */
    #ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
    #endif

    /* Close file over a execve() call. Used (over fcntl() with the FD_CLOEXEC
     * flag) to prevent a race condition in multithreaded programs where one
     * thread tries to set CLOEXEC with fcntl() at the same time another does
     * fork() then execve(). In this case, the file descriptor from open() can
     * be leaked to the program ran with execve() */
    #ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
    #endif

    return flags;
}


static int checkIntegrity(int fd)
{
    #ifdef makedev
    /* Device numbers of /dev/urandom */
    const unsigned int MAJOR_URANDOM_DEV_NUMBER = 1U;
    const unsigned int MINOR_URANDOM_DEV_NUMBER = 9U;
    
    dev_t deviceID;
    #endif

    struct stat info;

    /* Get information about file decriptor  */
    if (fstat(fd, &info) == -1)
        return 1;

    /* Test /dev/urandom as a character special file */
    if (!S_ISCHR(info.st_mode))
        return 1;

    /* Check authenticity of /dev/urandom, according to its device ID */
    #ifdef makedev
    deviceID = makedev(MAJOR_URANDOM_DEV_NUMBER, MINOR_URANDOM_DEV_NUMBER);

    if (info.st_rdev != deviceID)
        return 1;
    #endif

    return 0;
}


static size_t readRandomBytes(void *dest, size_t n, int fd)
{
    ssize_t tmpBytesRead;
    size_t totalBytesRead = 0;

    if (checkEntropy(fd, n))
        return 0;

    while (totalBytesRead < n)
    {
        tmpBytesRead = read(fd, (char *) dest + totalBytesRead, n - totalBytesRead);

        if (tmpBytesRead == -1)
        {
            /* If call has not been interrupted, it has failed */
            if (errno != EINTR)
                return totalBytesRead;
        }

        totalBytesRead += (size_t) tmpBytesRead;
    }

    return totalBytesRead;
}


static int checkEntropy(int fd, size_t n)
{
    int entropy;

    /* Get entropy count from the entropy_avail file in /proc */
    if (ioctl(fd, RNDGETENTCNT, &entropy) == -1 || entropy < 0)
        return 1;

    /* If insufficient entropy count */
    if ((unsigned) entropy < n * CHAR_BIT)
        return 1;

    return 0;
}