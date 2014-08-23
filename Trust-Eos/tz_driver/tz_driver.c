#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/random.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/mutex.h>

// trustzone/inclue/driver/tz_driver.h
#include "driver/tz_driver.h"
// trustzone/inclue/communication/types.h
#include "communication/types.h"
#include "communication/constants.h"

#define DEBUG_FUN       printk(KERN_ALERT "executing %s %d \n",__FILE__, __LINE__)

// 驱动幻数为T
#define TZ_MAGIC         'T'
#define DEVICE_NAME "trustzone_driver"

// 客户端通信数据
typedef union
{
    OPEN_SESSION_DATA open_session_data;
    CLOSE_SESSION_DATA close_session_data;
    INVOKE_SESSION_DATA invoke_session_data;
}COM_DATA;

// 客户端数据
typedef struct {
    // 通信数据 (OPEN or CLOSE or INVOKE)
    // COM_DATA com_data;
    // 客户端标识 （由驱动产生）
    TEEC_UUID uuid;
    // 登陆方法
    int login_type;
}CLIENT_DATA;

static dev_t dev_num;
static struct class *tz_class;
static struct device *tz_device;
static struct cdev tz_dev;
static struct mutex tz_mutex;

static int CallTZFunction(TZ_CMD_DATA *tz_cmd_data)
{
    volatile int ret = -1;  
   
    register uint32_t r0 asm("r0") = SMC_NS_ARG_TZ_API;
    register uint32_t r1 asm("r1") = (uint32_t)tz_cmd_data;
    register uint32_t r2 asm("r2") = 0;
    flush_cache_all();
    rmb();
    mutex_lock(&tz_mutex);
    r0 = SMC_NS_ARG_TZ_API;
    r1 = (uint32_t)tz_cmd_data;
    r2 = 0;
    do {
        asm volatile(
            __asmeq("%0", "r0")
            __asmeq("%1", "r0")
            __asmeq("%2", "r1")
            __asmeq("%3", "r2")
            "smc    #0  @ switch to secure world\n"
            : "=r" (r0)
            : "r" (r0), "r" (r1), "r" (r2));
    } while (0);

    ret = r0;
    mutex_unlock(&tz_mutex);
    wmb();
    return ret;
}

// user_open_session_data 是直接从用户空间传进来的，没有做任何处理
static int open_session(CLIENT_DATA *client_data, __user OPEN_SESSION_DATA *open_session_data)
{
    int ret = -EINVAL;

    TZ_CMD_DATA *tz_cmd_data = NULL;

    // 用户空间传递过来的参数
    OPEN_SESSION_DATA user_open_session_data;
    TZ_Operation user_tz_operation;
    TEEC_Param *user_params;

    // 从用户空间得到open_session_data
    if (0 != copy_from_user(&user_open_session_data,
                   open_session_data,
                   sizeof(OPEN_SESSION_DATA))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto open_finish_0;
    }
    if (0 != copy_from_user(&user_tz_operation, 
                    user_open_session_data.tz_operation,
                    sizeof(TZ_Operation))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto open_finish_0;
    }
    user_params = user_tz_operation.params;

    if (user_tz_operation.param_type != TEE_PARAM_TYPES(
                                      TEE_PARAM_TYPE_MEMREF_INPUT,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE)) {
        return TEEC_ERROR_BAD_PARAMETERS;
    }
    tz_cmd_data = (TZ_CMD_DATA *)kmalloc(sizeof(TZ_CMD_DATA), GFP_KERNEL);
    memset(tz_cmd_data, 0 ,sizeof(TZ_CMD_DATA));

    // params[0] 用于传递 目标UUID 类型为MEMREF_INPUT
    if (0 != copy_from_user(&tz_cmd_data->params[0].memref.size,
                   (void *)user_params[0].memref.size,
                   sizeof(uint32_t))) {
        printk(KERN_ALERT "get params[0].size from user failed!\n");
        goto open_finish_1;
    }
    tz_cmd_data->params[0].memref.buffer = 
             (void*)kmalloc(tz_cmd_data->params[0].memref.size, GFP_KERNEL);
    if (0 != copy_from_user(tz_cmd_data->params[0].memref.buffer,
                   user_params[0].memref.buffer, 
                   tz_cmd_data->params[0].memref.size)) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto open_finish_2;
    }
    // params[1] 用于传递 客户UUID 类型为MEMREF_INPUT
    tz_cmd_data->params[1].memref.buffer = &client_data->uuid;
    tz_cmd_data->params[1].memref.size = sizeof(TEEC_UUID);

    // params[2] 客户登陆类型
    tz_cmd_data->params[2].value.a = client_data->login_type;

    // 操作类型为open session
    tz_cmd_data->operation_type = TEE_OPERATION_OPEN_SESSION;
    tz_cmd_data->param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                             TEE_PARAM_TYPE_MEMREF_INPUT,
                                             TEE_PARAM_TYPE_VALUE_INPUT,
                                             TEE_PARAM_TYPE_NONE);
    // ENTRY CRITICAL SECTION
    ret = CallTZFunction(tz_cmd_data);
    // EXIT CRITICAL SECTION
    if (ret != TEEC_SUCCESS) {
        // 操作失败
        goto open_finish_2;
    }
    // 操作成功，将会话结果写回
    if (0 != copy_to_user(user_open_session_data.session_context,
                 &tz_cmd_data->session_context,
                 sizeof(SESSION_CONTEXT))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto open_finish_2;
    }
    // 将return_origin结果写回
    if ( NULL != user_open_session_data.return_origin &&
        0 != copy_to_user(user_open_session_data.return_origin,
                 &tz_cmd_data->return_origin,
                 sizeof(int32_t))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto open_finish_2;
    }

open_finish_2:
    kfree(tz_cmd_data->params[0].memref.buffer);
open_finish_1:
    kfree(tz_cmd_data);
open_finish_0:
    return ret;
}

static int invoke_session(CLIENT_DATA *client_data, __user INVOKE_SESSION_DATA *invoke_session_data)
{
    int ret = 0;
    int i;
    INVOKE_SESSION_DATA user_invoke_session_data;
    TZ_Operation user_tz_operation;
    TZ_CMD_DATA *tz_cmd_data = NULL;

    tz_cmd_data = (TZ_CMD_DATA *)kmalloc(sizeof(TZ_CMD_DATA), GFP_KERNEL);
    memset(tz_cmd_data, 0, sizeof(TZ_CMD_DATA));
    if (0 != copy_from_user(&user_invoke_session_data,
                    invoke_session_data,
                    sizeof(INVOKE_SESSION_DATA))) {
        DEBUG_FUN;
        ret = -1;
        goto invoke_finish_0;
    }
    if (0 != copy_from_user(&user_tz_operation,
                    user_invoke_session_data.tz_operation,
                    sizeof(TZ_Operation))) {
        DEBUG_FUN;
        ret = -1;
        goto invoke_finish_0;
    }

    // 从user拷贝参数params
    for (i=0; i<4; i++) {
        if (TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_VALUE_INPUT ||
            TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_VALUE_OUTPUT ||
            TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_VALUE_INOUT ) {
            // value.a 和 value.b 实际为用户空间传递进来的value.a和value.b的指针
            if (0 != copy_from_user(&tz_cmd_data->params[i].value.a,
                           (void *)user_tz_operation.params[i].value.a,
                           sizeof(uint32_t))) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
            if (0 != copy_from_user(&tz_cmd_data->params[i].value.b,
                           (void *)user_tz_operation.params[i].value.b,
                           sizeof(uint32_t))) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
        }
        else if (TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_MEMREF_INPUT ||
            TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_MEMREF_OUTPUT ||
            TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_MEMREF_INOUT ) {
            tz_cmd_data->params[i].memref.buffer = NULL;
            // 从用户空间赋值size
            if ( 0 != copy_from_user(&tz_cmd_data->params[i].memref.size,
                           (void *)user_tz_operation.params[i].memref.size,
                           sizeof(uint32_t))) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
            // 在内核中分配size的buffer
            tz_cmd_data->params[i].memref.buffer = 
                            kmalloc(tz_cmd_data->params[i].memref.size, GFP_KERNEL);
            // 从用户空间中拷贝size大小的数据
            if ( 0 != copy_from_user(tz_cmd_data->params[i].memref.buffer,
                           user_tz_operation.params[i].memref.buffer,
                           tz_cmd_data->params[i].memref.size)) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
        }
    }
    // param type
    tz_cmd_data->param_type = user_tz_operation.param_type;
    // operation type
    tz_cmd_data->operation_type = TEE_OPERATION_INVOKE_COMMAND;
    // session context
    if (0 != copy_from_user(&tz_cmd_data->session_context,
                   user_invoke_session_data.session_context,
                   sizeof(SESSION_CONTEXT))) {
        DEBUG_FUN;
        ret = -1;
        goto invoke_finish_1;
    }
    // command id
    tz_cmd_data->command_id = user_invoke_session_data.command;
    ret = CallTZFunction(tz_cmd_data);
    if (ret != TEEC_SUCCESS) {
        DEBUG_FUN;
        goto invoke_finish_1;
    }
    // 将操作结果写回到用户空间
    if (user_invoke_session_data.return_origin != NULL &&
        0 != copy_to_user(user_invoke_session_data.return_origin,
                 &tz_cmd_data->return_origin,
                 sizeof(uint32_t))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        DEBUG_FUN;
        goto invoke_finish_1;
    }
    for (i=0; i<4; i++) {
        if (TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_VALUE_OUTPUT ||
            TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_VALUE_INOUT ) {
            if (0 != copy_to_user((void *)user_tz_operation.params[i].value.a,
                         &tz_cmd_data->params[i].value.a, sizeof(uint32_t))) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
            if (0 != copy_to_user((void *)user_tz_operation.params[i].value.b,
                         &tz_cmd_data->params[i].value.b, sizeof(uint32_t))) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
        }
        else if (TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_MEMREF_OUTPUT ||
            TEE_PARAM_TYPE_GET(user_tz_operation.param_type,i) == TEE_PARAM_TYPE_MEMREF_INOUT ) {
            // 写回新的size
            if (0 != copy_to_user((void*)user_tz_operation.params[i].memref.size,
                         &tz_cmd_data->params[i].memref.size,
                         sizeof(uint32_t))) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
            // 写回新的buffer
            if (0 != copy_to_user(user_tz_operation.params[i].memref.buffer,
                        tz_cmd_data->params[i].memref.buffer,
                        tz_cmd_data->params[i].memref.size)) {
                DEBUG_FUN;
                ret = -1;
                goto invoke_finish_1;
            }
            kfree(tz_cmd_data->params[i].memref.buffer);
            tz_cmd_data->params[i].memref.buffer = NULL;
        }
    }
    kfree(tz_cmd_data);
    return ret;
invoke_finish_1:
    for (i=0 ;i<4; i++) {
        if (TEE_PARAM_TYPE_GET(tz_cmd_data->param_type,i) == TEE_PARAM_TYPE_MEMREF_OUTPUT ||
            TEE_PARAM_TYPE_GET(tz_cmd_data->param_type,i) == TEE_PARAM_TYPE_MEMREF_INOUT) {
            if (tz_cmd_data->params[i].memref.buffer != NULL) {
                kfree(tz_cmd_data->params[i].memref.buffer);
                tz_cmd_data->params[i].memref.buffer = NULL;
            }
        }
    }
invoke_finish_0:
    kfree(tz_cmd_data);
    tz_cmd_data = NULL;
    return ret;
}

static int close_session(CLIENT_DATA *client_data, __user CLOSE_SESSION_DATA *close_session_data)
{
    int ret = -EINVAL;
    CLOSE_SESSION_DATA user_close_session_data;
    SESSION_CONTEXT user_session_context;
    TZ_CMD_DATA *tz_cmd_data = NULL;
    if (0 != copy_from_user(&user_close_session_data,
                   close_session_data,
                   sizeof(CLOSE_SESSION_DATA))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto close_finish_0;
    }
    if (0 != copy_from_user(&user_session_context,
                   user_close_session_data.session_context,
                   sizeof(SESSION_CONTEXT))) {
        ret = TEEC_ERROR_BAD_PARAMETERS;
        goto close_finish_0;
    }
    tz_cmd_data = (TZ_CMD_DATA *)kmalloc(sizeof(TZ_CMD_DATA), GFP_KERNEL);
    memset(tz_cmd_data, 0, sizeof(TZ_CMD_DATA));
    tz_cmd_data->operation_type = TEE_OPERATION_CLOSE_SESSION;
    tz_cmd_data->session_context = user_session_context;
    ret = CallTZFunction(tz_cmd_data);

//close_finish_1:
    kfree(tz_cmd_data);
close_finish_0:
    return ret;
}


static int tz_release(struct inode *inode, struct file *file)
{

    if (file->private_data) {
        // display_uuid((unsigned char*)file->private_data);
        kfree(file->private_data);
    }
    else {
        printk(KERN_ALERT "file.private_data is null");
    }
    return 0;
}

static int tz_open(struct inode *inode, struct file *file)
{
    
    CLIENT_DATA *client_data = NULL;
    //inode = inode;
    //file = file;

    client_data = (CLIENT_DATA *)kmalloc(sizeof(CLIENT_DATA),GFP_KERNEL);
    memset(client_data, 0, sizeof(CLIENT_DATA));
    generate_random_uuid((unsigned char *)&client_data->uuid);
    file->private_data = client_data;
    // display_uuid((unsigned char *)&client_data->uuid);
    return 0;
}
static long tz_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;
    CLIENT_DATA *client_data = (CLIENT_DATA *)file->private_data;
    if (_IOC_TYPE(cmd) != TZ_MAGIC) {
        printk(KERN_ALERT "error :tz_ioctl() invalid magic num\n");
        return -EINVAL;
    }
    if (_IOC_NR(cmd) > TZ_MAX_NR) {
        printk(KERN_ALERT "error :tz_ioctl() larger than max nr\n");
        return -EINVAL;
    }

    switch (cmd) {
        case TZ_IOCTL_OPEN_SESSION:
            return open_session(client_data, 
                               (__user OPEN_SESSION_DATA *)arg);
            break;
        case TZ_IOCTL_CLOSE_SESSION:
            return close_session(client_data,
                                (__user CLOSE_SESSION_DATA *)arg);
            break;
        case TZ_IOCTL_INVOKE_SESSION:
            return invoke_session(client_data, (__user INVOKE_SESSION_DATA *)arg);
            break;
        default:
            printk("KERN_ALERT error :tz_ioctl() not found a valid command\n");
            return -EINVAL;
            break;
    }
    return ret;
}

static struct file_operations dev_fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl = tz_ioctl,
    .open = tz_open,
    .release = tz_release,
};

static int __init tz_init(void)
{
    int ret = -1;
    printk(KERN_ALERT "Hello trustzone driver\n");
    mutex_init(&tz_mutex); 
    // 分配设备号
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret !=0 ) {
        printk(KERN_ALERT "alloc_chrdev_region() failed\n");
        goto tz_init_fail_0;
    }
    // 注册一个类，使该驱动可以在/dev/目录下面建立一个节点
    tz_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(tz_class)) {
        ret = -1;
        goto tz_init_fail_1;
    }
    // 创建一个设备节点，节点名为DEVICE_NAME
    tz_device = device_create(tz_class, NULL, dev_num,
                              NULL, DEVICE_NAME);
    if (tz_device == NULL) {
        ret = -1;
        printk(KERN_ALERT "device_create() failed\n");
        goto tz_init_fail_2;
    }

    // 绑定文件操作到设备
    cdev_init(&tz_dev, &dev_fops);
    ret = cdev_add(&tz_dev, dev_num, 1);
    if (ret != 0) {
        printk(KERN_ALERT "cdev_add() failed\n");
        goto tz_init_fail_3;
    }
    ret = 0;
    return ret;


//tz_init_fail_4:
    cdev_del(&tz_dev);
tz_init_fail_3:
    device_destroy(tz_class, dev_num);
tz_init_fail_2:
    class_destroy(tz_class);
tz_init_fail_1:
    unregister_chrdev_region(dev_num, 1);
tz_init_fail_0:
    mutex_destroy(&tz_mutex);
    return ret;
}

static void __exit tz_exit(void)
{
    printk(KERN_ALERT "Goodbye trustzone driver\n");
    cdev_del(&tz_dev);
    device_destroy(tz_class, dev_num);
    class_destroy(tz_class);
    unregister_chrdev_region(dev_num, 1);
    mutex_destroy(&tz_mutex);
}


module_init(tz_init);
module_exit(tz_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("T-OS TRUSTZONE DRIVER");