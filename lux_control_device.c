#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/serdev.h>
#include <linux/fs.h>
#include <linux/of_device.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/types.h> 

// Autor del modulo
#define AUTHOR		"Franco Lopez"
// Char device name
#define CDEV_NAME	"lux_control"
// Minor number del char device
#define CDEV_MINOR	50
// Cantidad de devices para reservar
#define CDEV_COUNT	1
// Cantidad maxima de bytes para el buffer de usuario
#define SHARED_BUFF_SIZE	64

// IDs de serial devices
static struct of_device_id serdev_ids[] = {
	{ .compatible = "brightlight,lux_control", },
	{ }
};

MODULE_DEVICE_TABLE(of, serdev_ids);

static int lux_control_probe(struct serdev_device *serdev);

// Puntero global para UART
static struct serdev_device *g_serdev = NULL;

/**
 * @brief Al recibir un mensaje por la UART se ejecuta esta funcion
 * 
 */
static size_t lux_control_recv(struct serdev_device *serdev, const u8 *buffer, size_t size){
    static char str[SHARED_BUFF_SIZE] = {0};

    static int i=0;

    if(*buffer){
        str[i++] = *buffer;
    }

    if(i == SHARED_BUFF_SIZE || str[i-1] == '\0'){
       printk(KERN_INFO "%s: Se recibieron %d bytes por UART. El mensaje fue '%s'\n", AUTHOR, i-1, str);
		// Reinicio las variables
		memset(str, 0, i);
		i = 0;
    }

    return size;
}

/*====================================================================================================
                            DEVICE OPERATIONS
  ====================================================================================================*/

static const struct serdev_device_ops lux_control_uart_ops = {
    .receive_buf = lux_control_recv,
};

/**
 * @brief Se llama cuando se conecta un dispositivo UART
 * 
 */
static int lux_control_probe(struct serdev_device *serdev){
	printk(KERN_INFO "%s: Se conecto un dispositivo UART\n", AUTHOR);

    if(serdev_device_open(serdev)){
        printk(KERN_ERR "%s: Error abriendo el puerto UART\n", AUTHOR);
        return -1;
    }

    serdev_device_set_baudrate(serdev, 9600);
    serdev_device_set_flow_control(serdev, false);
    serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

    g_serdev = serdev;
    if(g_serdev == NULL){
        printk(KERN_ERR "%s: Algo salio mal con el puerto UART\n", AUTHOR);
        return -1;
    }

    return 0;
}

/**
 * @brief Se llama cuando se desconecta el dispositivo
 * 
 */
static void lux_control_uart_remove(struct serdev_device *serdev){
    printk(KERN_INFO "%s: Cerrando UART\n", AUTHOR);
    serdev_device_close(serdev);
}

static struct serdev_device_driver lux_control_driver = {
    .probe = lux_control_probe,
    .remove = lux_control_uart_remove,
    .driver = {
        .name = "lux_control",
        .of_match_table = serdev_ids,
    },
};


/*====================================================================================================
                            CHAR DEVICE
  ====================================================================================================*/

typedef struct {
  struct cdev cdev;
  dev_t cdev_number;
  unsigned int cdev_major;
  struct class *cdev_class;
} lux_control_cdev_t;

lux_control_cdev_t lux_control_cdev;

char shared_buffer[SHARED_BUFF_SIZE];

static ssize_t on_read(struct file *f, char __user *buff, size_t size, loff_t *off){
    int not_copied, to_copy = (size > SHARED_BUFF_SIZE) ? SHARED_BUFF_SIZE : size;

    if(*off >= to_copy) {return 0;}

    not_copied = copy_to_user(shared_buffer, buff, to_copy);

    printk("%s: Leido sobre /dev/%s - %s\n", AUTHOR, CDEV_NAME, shared_buffer);

    *off = to_copy - not_copied;

    return to_copy - not_copied;
}

static ssize_t on_write(struct file *f, const char __user *buff, size_t size, loff_t *off){
    int not_copied, to_copy = (size > SHARED_BUFF_SIZE - 1 )?SHARED_BUFF_SIZE - 1 : size;

    if(to_copy == 0){
        printk("Nada que enviar....\n");
        return 0;
    }

    not_copied = copy_from_user(shared_buffer, buff, to_copy);

    if (not_copied > 0) {
        // Devuelve EFAULT si no se pudo copiar todo
        return -EFAULT;
    }

    shared_buffer[to_copy] = '\0';

    
    if(g_serdev != NULL){
        printk("%s: Escrito sobre /dev/%s - %s - not copy: %d - to copy: %d\n", AUTHOR, CDEV_NAME, shared_buffer, not_copied, to_copy);
        serdev_device_write_buf(g_serdev, shared_buffer, sizeof(shared_buffer));
    }

    return to_copy;
}

const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = on_read,
    .write = on_write,
};

/*====================================================================================================
                                    INIT
  ====================================================================================================*/


static int __init lux_control_init(void){
    if(alloc_chrdev_region(&lux_control_cdev.cdev_number, CDEV_MINOR, CDEV_COUNT, CDEV_NAME ) < 0){
        printk(KERN_ERR "%s> No se pudo crear el char device \n", AUTHOR);
        return -1;
    }

    printk(KERN_INFO "%s: Char device creado en major %d, minor %d\n", AUTHOR, MAJOR(lux_control_cdev.cdev_number), MINOR(lux_control_cdev.cdev_number));

    cdev_init(&lux_control_cdev.cdev, &fops);

    lux_control_cdev.cdev.owner = THIS_MODULE;

    if(cdev_add(&lux_control_cdev.cdev, lux_control_cdev.cdev_number, CDEV_COUNT) < 0){
        unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
        printk(KERN_ERR "%s: No se pudo crear el char device\n", AUTHOR);
        return -1;
    }

    lux_control_cdev.cdev_class = class_create(CDEV_NAME);

    if(IS_ERR(lux_control_cdev.cdev_class)) {
		// Error
		printk(KERN_ERR "%s: No se pudo crear la clase del char device\n", AUTHOR);
		class_destroy(lux_control_cdev.cdev_class);
		unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
		return -1;
    }
    
    if(IS_ERR(device_create(lux_control_cdev.cdev_class, NULL, lux_control_cdev.cdev_number, NULL, CDEV_NAME))){
        printk(KERN_ERR "%s: No se pudo crear el archivo del char device\n", AUTHOR);
        class_destroy(lux_control_cdev.cdev_class);
        unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
        return -1;
    }

    printk(KERN_INFO "%s: Archivo de char device creado!\n", AUTHOR);

    if(serdev_device_driver_register(&lux_control_driver)){
        printk(KERN_ERR "%s: No se pudo crear el driver de UART\n", AUTHOR);
        return -1;
    }

    printk(KERN_INFO "%s: Driver para UART registrado!\n", AUTHOR);
    return 0;
}

static void __exit lux_control_exit(void){
	printk("%s: Driver deshabilitado\n", AUTHOR);
	device_destroy(lux_control_cdev.cdev_class, lux_control_cdev.cdev_number);
	class_destroy(lux_control_cdev.cdev_class);
	unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
	cdev_del(&lux_control_cdev.cdev);
	serdev_device_driver_unregister(&lux_control_driver);
}

// Registro funciones de inicializacion y saiida del driver
module_init(lux_control_init);
module_exit(lux_control_exit);

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("Modulo que inicializa y registra un char device");
