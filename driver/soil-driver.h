#ifndef SOIL_CHAR_DRIVER_SOILCHAR_H_
#define SOIL_CHAR_DRIVER_SOILCHAR_H_

// #define SOIL_DEBUG 1

#undef PDEBUG
#ifdef SOIL_DEBUG
#  ifdef __KERNEL__
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "soilchar: " fmt, ## args)
#  else
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct soil_dev
{
    struct cdev cdev;     /* Char device structure      */
};


#endif 
