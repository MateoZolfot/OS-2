#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/string.h>

#define DEV_NAME "sysinfo"

static int major;
static char msg[256];
static size_t msg_len;
static int current_mode = 0; // 0:all, 1:kernel, 2:cpu, 3:time

static ssize_t sysinfo_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
    if (*off >= msg_len) return 0;
    if (len > msg_len - *off) len = msg_len - *off;
    if (copy_to_user(buf, msg + *off, len)) return -EFAULT;
    *off += len;
    return len;
}

static ssize_t sysinfo_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {
    char cmd[16];
    size_t copy_len = len < sizeof(cmd) - 1 ? len : sizeof(cmd) - 1;

    if (copy_from_user(cmd, buf, copy_len)) return -EFAULT;
    cmd[copy_len] = '\0';

    if (strncmp(cmd, "all", 3) == 0) current_mode = 0;
    else if (strncmp(cmd, "kernel", 6) == 0) current_mode = 1;
    else if (strncmp(cmd, "cpu", 3) == 0) current_mode = 2;
    else if (strncmp(cmd, "time", 4) == 0) current_mode = 3;
    
    return len;
}

static int sysinfo_open(struct inode *i, struct file *f) {
    if (current_mode == 1) msg_len = scnprintf(msg, sizeof(msg), "release=%s\n", utsname()->release);
    else if (current_mode == 2) msg_len = scnprintf(msg, sizeof(msg), "cpus=%u\n", num_online_cpus());
    else if (current_mode == 3) msg_len = scnprintf(msg, sizeof(msg), "jiffies=%llu\n", (unsigned long long)jiffies);
    else msg_len = scnprintf(msg, sizeof(msg), "release=%s\njiffies=%llu\ncpus=%u\n", utsname()->release, (unsigned long long)jiffies, num_online_cpus());
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = sysinfo_open,
    .read = sysinfo_read,
    .write = sysinfo_write,
};

static int __init sysinfo_init(void) {
    major = register_chrdev(0, DEV_NAME, &fops);
    if (major < 0) return major;
    pr_info("sysinfo: registered major=%d\n", major);
    return 0;
}

static void __exit sysinfo_exit(void) {
    unregister_chrdev(major, DEV_NAME);
    pr_info("sysinfo: unregistered\n");
}

module_init(sysinfo_init);
module_exit(sysinfo_exit);
MODULE_LICENSE("GPL");
