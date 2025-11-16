// #include <linux/kernel.h>
// #include <linux/module.h>
// #include <linux/init.h>
// #include <linux/cdev.h>
// #include <linux/device.h>
// #include <linux/serdev.h>
// #include <linux/fs.h>
// #include <linux/of_device.h>
// #include <linux/uaccess.h>
// #include <linux/of.h>
// #include <linux/types.h> 

// // Autor del modulo
// #define AUTHOR		"Franco Lopez"
// // Char device name
// #define CDEV_NAME	"lux_control"
// // Minor number del char device
// #define CDEV_MINOR	50
// // Cantidad de devices para reservar
// #define CDEV_COUNT	1
// // Cantidad maxima de bytes para el buffer de usuario
// #define SHARED_BUFF_SIZE	64

// // IDs de serial devices
// static struct of_device_id serdev_ids[] = {
// 	{ .compatible = "brightlight,lux_control", },
// 	{ }
// };

// MODULE_DEVICE_TABLE(of, serdev_ids);

// static int lux_control_probe(struct serdev_device *serdev);

// // Puntero global para UART
// static struct serdev_device *g_serdev = NULL;

// /**
//  * @brief Al recibir un mensaje por la UART se ejecuta esta funcion
//  * 
//  */
// static size_t lux_control_recv(struct serdev_device *serdev, const u8 *buffer, size_t size){
//     static char str[SHARED_BUFF_SIZE] = {0};

//     static int i=0;

//     if(*buffer){
//         str[i++] = *buffer;
//     }

//     if(i == SHARED_BUFF_SIZE || str[i-1] == '\0'){
//        printk(KERN_INFO "%s: Se recibieron %d bytes por UART. El mensaje fue '%s'\n", AUTHOR, i-1, str);
// 		// Reinicio las variables
// 		memset(str, 0, i);
// 		i = 0;
//     }

//     return size;
// }

// /*====================================================================================================
//                             DEVICE OPERATIONS
//   ====================================================================================================*/

// static const struct serdev_device_ops lux_control_uart_ops = {
//     .receive_buf = lux_control_recv,
// };

// /**
//  * @brief Se llama cuando se conecta un dispositivo UART
//  * 
//  */
// static int lux_control_probe(struct serdev_device *serdev){
// 	printk(KERN_INFO "%s: Se conecto un dispositivo UART\n", AUTHOR);

//     // Se asignan las operaciones del UART
//     serdev_device_set_client_ops(serdev, &lux_control_recv);

//     if(serdev_device_open(serdev)){
//         printk(KERN_ERR "%s: Error abriendo el puerto UART\n", AUTHOR);
//         return -1;
//     }

//     serdev_device_set_baudrate(serdev, BAUD_RATE);
//     serdev_device_set_flow_control(serdev, false);
//     serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

//     g_serdev = serdev;
//     if(g_serdev == NULL){
//         printk(KERN_ERR "%s: Algo salio mal con el puerto UART\n", AUTHOR);
//         return -1;
//     }

//     return 0;
// }

// /**
//  * @brief Se llama cuando se desconecta el dispositivo
//  * 
//  */
// static void lux_control_uart_remove(struct serdev_device *serdev){
//     printk(KERN_INFO "%s: Cerrando UART\n", AUTHOR);
//     serdev_device_close(serdev);
// }

// static struct serdev_device_driver lux_control_driver = {
//     .probe = lux_control_probe,
//     .remove = lux_control_uart_remove,
//     .driver = {
//         .name = "lux_control",
//         .of_match_table = serdev_ids,
//     },
// };


// /*====================================================================================================
//                             CHAR DEVICE
//   ====================================================================================================*/

// typedef struct {
//   struct cdev cdev;
//   dev_t cdev_number;
//   unsigned int cdev_major;
//   struct class *cdev_class;
// } lux_control_cdev_t;

// lux_control_cdev_t lux_control_cdev;

// char shared_buffer[SHARED_BUFF_SIZE];

// static ssize_t on_read(struct file *f, char __user *buff, size_t size, loff_t *off){
//     int not_copied, to_copy = (size > SHARED_BUFF_SIZE) ? SHARED_BUFF_SIZE : size;

//     if(*off >= to_copy) {return 0;}

//     not_copied = copy_to_user(shared_buffer, buff, to_copy);

//     printk("%s: Leido sobre /dev/%s - %s\n", AUTHOR, CDEV_NAME, shared_buffer);

//     *off = to_copy - not_copied;

//     return to_copy - not_copied;
// }

// static ssize_t on_write(struct file *f, const char __user *buff, size_t size, loff_t *off){
//     int not_copied, to_copy = (size > SHARED_BUFF_SIZE - 1 )?SHARED_BUFF_SIZE - 1 : size;

//     if(to_copy == 0){
//         printk("Nada que enviar....\n");
//         return 0;
//     }

//     not_copied = copy_from_user(shared_buffer, buff, to_copy);

//     if (not_copied > 0) {
//         // Devuelve EFAULT si no se pudo copiar todo
//         return -EFAULT;
//     }

//     shared_buffer[to_copy] = '\0';

    
//     if(g_serdev != NULL){
//         printk("%s: Escrito sobre /dev/%s - %s - not copy: %d - to copy: %d\n", AUTHOR, CDEV_NAME, shared_buffer, not_copied, to_copy);
//         serdev_device_write_buf(g_serdev, shared_buffer, sizeof(shared_buffer));
//     }

//     return to_copy;
// }

// const struct file_operations fops = {
//     .owner = THIS_MODULE,
//     .read = on_read,
//     .write = on_write,
// };

// /*====================================================================================================
//                                     INIT
//   ====================================================================================================*/


// static int __init lux_control_init(void){
//     if(alloc_chrdev_region(&lux_control_cdev.cdev_number, CDEV_MINOR, CDEV_COUNT, CDEV_NAME ) < 0){
//         printk(KERN_ERR "%s> No se pudo crear el char device \n", AUTHOR);
//         return -1;
//     }

//     printk(KERN_INFO "%s: Char device creado en major %d, minor %d\n", AUTHOR, MAJOR(lux_control_cdev.cdev_number), MINOR(lux_control_cdev.cdev_number));

//     cdev_init(&lux_control_cdev.cdev, &fops);

//     lux_control_cdev.cdev.owner = THIS_MODULE;

//     if(cdev_add(&lux_control_cdev.cdev, lux_control_cdev.cdev_number, CDEV_COUNT) < 0){
//         unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
//         printk(KERN_ERR "%s: No se pudo crear el char device\n", AUTHOR);
//         return -1;
//     }

//     lux_control_cdev.cdev_class = class_create(CDEV_NAME);

//     if(IS_ERR(lux_control_cdev.cdev_class)) {
// 		// Error
// 		printk(KERN_ERR "%s: No se pudo crear la clase del char device\n", AUTHOR);
// 		class_destroy(lux_control_cdev.cdev_class);
// 		unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
// 		return -1;
//     }
    
//     if(IS_ERR(device_create(lux_control_cdev.cdev_class, NULL, lux_control_cdev.cdev_number, NULL, CDEV_NAME))){
//         printk(KERN_ERR "%s: No se pudo crear el archivo del char device\n", AUTHOR);
//         class_destroy(lux_control_cdev.cdev_class);
//         unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
//         return -1;
//     }

//     printk(KERN_INFO "%s: Archivo de char device creado!\n", AUTHOR);

//     if(serdev_device_driver_register(&lux_control_driver)){
//         printk(KERN_ERR "%s: No se pudo crear el driver de UART\n", AUTHOR);
//         return -1;
//     }

//     printk(KERN_INFO "%s: Driver para UART registrado!\n", AUTHOR);
//     return 0;
// }

// static void __exit lux_control_exit(void){
// 	printk("%s: Driver deshabilitado\n", AUTHOR);
// 	device_destroy(lux_control_cdev.cdev_class, lux_control_cdev.cdev_number);
// 	class_destroy(lux_control_cdev.cdev_class);
// 	unregister_chrdev_region(lux_control_cdev.cdev_number, CDEV_COUNT);
// 	cdev_del(&lux_control_cdev.cdev);
// 	serdev_device_driver_unregister(&lux_control_driver);
// }

// // Registro funciones de inicializacion y saiida del driver
// module_init(lux_control_init);
// module_exit(lux_control_exit);

// /* Meta Information */
// MODULE_LICENSE("GPL");
// MODULE_AUTHOR(AUTHOR);
// MODULE_DESCRIPTION("Modulo que inicializa y registra un char device");
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/serdev.h>
#include <linux/fs.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/spinlock.h>

// Autor del modulo
#define AUTHOR              "LUX_CONTROL"
// Nombre del char device
#define CHRDEV_NAME         "lux_control"
// Minor number del char device
#define CHRDEV_MINOR        1
// Cantidad de char devices
#define CHRDEV_COUNT        1
// Cantidad de caracteres maximos en el buffer
#define SHARED_BUFFER_SIZE  64

#define UART_DATA_BUFFER_SIZE 1024
// Baud rate del UART
#define BAUD_RATE           115200
// Paridad
#define PARITY              SERDEV_PARITY_NONE

// Variable que guarda los major y minor numbers del char device
static dev_t chrdev_number;
// Variable que representa el char device
static struct cdev chrdev;
// Clase del char device
static struct class *chrdev_class;
// ID
static struct of_device_id serdev_ids[] = {
    { .compatible = "brightlight,lux_control", },
    {}
};
MODULE_DEVICE_TABLE(of, serdev_ids);
// Serdev Device
static struct serdev_device *g_serdev = NULL;
// Buffer de datos para compartir entre user y kernel
static char shared_buffer[SHARED_BUFFER_SIZE];
static size_t recibido = 0, recibido_size = 0;
static wait_queue_head_t waitqueue;
static char uart_buff[UART_DATA_BUFFER_SIZE];
int uart_buff_index;
spinlock_t uart_lock;

// Prototipos de los callbacks de fops
static ssize_t dev_on_read(struct file *f, char __user *buff, size_t size, loff_t *off);
static ssize_t dev_on_write(struct file *f, const char __user *buff, size_t size, loff_t *off);
// Prototipos de los callbacks del driver uart 
static int dev_probe(struct serdev_device *serdev);
static void dev_uart_remove(struct serdev_device *serdev);
// Prototipos de las operaciones del UART
static size_t dev_uart_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size);

// Operaciones de archivos del char device
static struct file_operations chrdev_ops = {
    .owner = THIS_MODULE,
    .read = dev_on_read,
    .write = dev_on_write
};

// Operaciones del driver uart
static struct serdev_device_driver egb_uart_driver = {
    .probe = dev_probe,
    .remove = dev_uart_remove,
    .driver = {
        .name = "egb_uart",
        .of_match_table = serdev_ids,
    }
};

// Operaciones del UART
static const struct serdev_device_ops egb_uart_ops = {
    .receive_buf = dev_uart_recv,
};

/**
 * @brief Operacion si se lee el char device
 */
static ssize_t dev_on_read(struct file *f, char __user *buff, size_t size, loff_t *off) {
    int not_copied;
    size_t bytes_to_copy;
    unsigned long flags; // Para spin_lock_irqsave

    if (*off > 0) {
        // Resetea el offset para la próxima llamada open/write
        *off = 0; 
        return 0; // Devuelve 0 para indicar EOF 
    }

    // 1. Bloqueamos hasta que el flag 'recibido' sea TRUE
    // (Esta espera debe estar FUERA del spinlock)
    if (wait_event_interruptible(waitqueue, recibido == 1)) {
        // Si el usuario presiona Ctrl+C o recibe una señal
        return -ERESTARTSYS;
    }

    // --- CRITICAL SECTION START ---
    spin_lock_irqsave(&uart_lock, flags);

    // 2. Comprobar de nuevo (la condición de salida de la espera) y validar
    if (recibido != 1) {
        // Esto no debería suceder después de wait_event, pero es una buena práctica de protección
        spin_unlock_irqrestore(&uart_lock, flags);
        return 0; 
    }

    // 3. Determinar el tamaño de la copia (máximo la línea recibida o el buffer del usuario)
    bytes_to_copy = min(recibido_size, size);

    // 4. Resetear el estado de recepción ANTES de copiar (pero DENTRO del lock)
    // El tamaño de la siguiente lectura será 0 hasta que llegue el próximo dato
    recibido = 0;
    recibido_size = 0;
    
    // 5. Finalizar la sección crítica antes de copy_to_user (que puede tomar tiempo)
    spin_unlock_irqrestore(&uart_lock, flags);
    // --- CRITICAL SECTION END ---

    // 6. Copiamos al usuario (FUERA del lock)
    // Usamos uart_buff, que ahora está "congelado" y protegido de dev_uart_recv 
    // por el flag de 'recibido' = 0;
    not_copied = copy_to_user(buff, uart_buff, bytes_to_copy);

    // 7. Calcular y devolver el resultado
    if (not_copied) {
        printk(KERN_WARNING "%s: Solo se copiaron %zu bytes de %zu a espacio de usuario.\n",
               AUTHOR, bytes_to_copy - not_copied, bytes_to_copy);
    }

    // El offset solo se actualiza si el driver está diseñado para lecturas secuenciales
    *off += (bytes_to_copy - not_copied); 

    // Imprimir el dato, útil para depuración
    // Nota: printk aquí sigue siendo menos seguro que usar un workqueue, pero es funcional.
    printk(KERN_INFO "%s: Leido del char device (%zu bytes)\n", AUTHOR, bytes_to_copy - not_copied);

    for(int i=0; i<UART_DATA_BUFFER_SIZE;i++){
        uart_buff[i]='\0';
    }

    return bytes_to_copy - not_copied;
}

/**
 * @brief Operacion si se escribe el char device
 */
static ssize_t dev_on_write(struct file *f, const char __user *buff, size_t size, loff_t *off) {
    // Variables auxiliares
    int to_copy, not_copied, len;
    // Se fija cuanto puede copiar sin exceder el shared buffer
    to_copy = min(size, sizeof(shared_buffer) - 1);
    // Copia del user space al kernel space, devuelve cuanto no se copio
    not_copied = copy_from_user(shared_buffer, buff, to_copy);
    // Guardamos la cantidad de datos recibidos:
    len = to_copy - not_copied;
    // Usamos otra variable para hacer el printk pero enviar el dato con el \n
    char printk_buff[SHARED_BUFFER_SIZE];
    memcpy(printk_buff, shared_buffer, len);
    printk_buff[len] = '\0';
    if(len > 0 && printk_buff[len - 1] == '\n') printk_buff[len - 1] = '\0';
    // Hago un print de lo que se escribio efectivamente
    printk("%s: Escrito sobre /dev/%s - %s\n", AUTHOR, CHRDEV_NAME, printk_buff);
    // Se verifica la UART
    if(g_serdev != NULL) {
        // Se envia al UART
        serdev_device_write_buf(g_serdev, shared_buffer, len);
        // Se devuelve cuanto se copio
        return to_copy - not_copied;
    }
    // Retorna 0 si no hay UART
    return 0;
}

/**
 * @brief Operacion si se detecta UART. Crea el serdev device y le asigna las operaciones
 * @return Devuelve cero si la inicializacion fue correcta
 */
static int dev_probe(struct serdev_device *serdev) {
    printk(KERN_INFO "%s: Se conecto UART\n", AUTHOR);
    // Se asignan las operaciones del UART
    serdev_device_set_client_ops(serdev, &egb_uart_ops);
    // Se intenta abrir el UART
    if(serdev_device_open(serdev)) {
        printk(KERN_ERR "%s: Error abriendo el UART\n", AUTHOR);
        return -1;
    }
    // Configuracion de UART
    serdev_device_set_baudrate(serdev, BAUD_RATE);
    serdev_device_set_flow_control(serdev, false);
    serdev_device_set_parity(serdev, PARITY);
    // Guardamos el punto al serdev device
    g_serdev = serdev;
    if(g_serdev == NULL) {
        printk(KERN_ERR "%s: Error configurando el UART\n", AUTHOR);
        return -1;
    }
    return 0;
}

/**
 * @brief Operacion si se remueve UART. Cierra el serdev device.
 */
static void dev_uart_remove(struct serdev_device *serdev) {
    printk(KERN_INFO "%s: UART cerrada\n", AUTHOR);
    // Se cierra el UART
    serdev_device_close(serdev);
}

/**
 * @brief Operacion si se reciben caracteres de UART
 */
static size_t dev_uart_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size) {
    // printk("Size: %ld\n", size);
	// int to_copy = min(size, SHARED_BUFFER_SIZE - 1);
    int line_complete = 0;
    int i;

    for( i = 0; i < size; i++){
        char c = buffer[i];

        if(uart_buff_index > UART_DATA_BUFFER_SIZE){// termina la cadena
            printk(KERN_WARNING "%s: Buffer de línea lleno, descartando dato\n", AUTHOR);
            uart_buff_index = 0;
            // limpio buffer
        }
        
        uart_buff[uart_buff_index] = c;
        uart_buff_index++;
        if(c == '\n'){
            line_complete = 1;
            break;
        }
    }

    if(line_complete){
        uart_buff[uart_buff_index - 1] = '\0';

        printk(KERN_INFO "%s: Línea completa recibida por UART: '%s'\n", AUTHOR, uart_buff);

        recibido = 1;
        recibido_size = uart_buff_index - 1; // Ajusta el tamaño reportado
        wake_up_interruptible(&waitqueue);

        uart_buff_index = 0;
    }

	// memcpy(shared_buffer, buffer, to_copy);
	// shared_buffer[to_copy] = '\0';
	// recibido_size = to_copy;
	// recibido = 1;
	// wake_up_interruptible(&waitqueue);
    
    // for(int i=uart_buff_index; recibido_size>0 && i<UART_DATA_BUFFER_SIZE ; i++, recibido_size--, uart_buff_index++){
    //     uart_buff[i] = shared_buffer[];
    //     if(shared_buffer[i] == '\n'){
    //         uart_buff[i+1] = '\0';
    //         printk(KERN_INFO "%s: Recibido por UART: '%s'\n", AUTHOR, uart_buff);
    //     }
    // }
    return size;
}

/**
 * @brief Crea el char device
 * @return Devuelve cero si la inicializacion fue correcta
 */
static int __init module_kernel_init(void) {
    init_waitqueue_head(&waitqueue);
    // Reservar char device
    if(alloc_chrdev_region(&chrdev_number, CHRDEV_MINOR, CHRDEV_COUNT, CHRDEV_NAME) < 0) {
        printk(KERN_ERR "%s: No se pudo crear el char device\n", AUTHOR);
        return -1;
    }
    // Mensaje para buscar el char device
    printk(KERN_INFO "%s: Se reservo char device con major %d y minor %d\n", AUTHOR, MAJOR(chrdev_number), MINOR(chrdev_number));
    // Inicializa el char device y sus operaciones de archivos
    cdev_init(&chrdev, &chrdev_ops);
    // Asocia el char device a la zona reservada
    if(cdev_add(&chrdev, chrdev_number, CHRDEV_COUNT) < 0) {
        unregister_chrdev_region(chrdev_number, CHRDEV_COUNT);
        printk(KERN_ERR "%s: No se pudo crear el char device\n", AUTHOR);
        return -1;
    }
    // Crea la estructura de clase
    chrdev_class = class_create(AUTHOR);
    // Verifica error
    if(IS_ERR(chrdev_class)) {
        unregister_chrdev_region(chrdev_number, CHRDEV_COUNT);
        printk(KERN_ERR "%s: No se pudo crear el char device\n", AUTHOR);
        return -1;
    }
    // Se crea el archivo del char device
    if(IS_ERR(device_create(chrdev_class, NULL, chrdev_number, NULL, CHRDEV_NAME))) {
        class_destroy(chrdev_class);
        unregister_chrdev_region(chrdev_number, CHRDEV_COUNT);
        printk(KERN_ERR "%s: No se pudo crear el char device\n", AUTHOR);
        return -1;
    }
    // Registro driver para UART
    if(serdev_device_driver_register(&egb_uart_driver)) {
        printk(KERN_ERR "%s: No se pudo crear el driver de UART\n", AUTHOR);
        return -1;
    }
    // Mensaje de correcta finalizacion
    printk(KERN_INFO "%s: Fue creado el char device y driver UART\n", AUTHOR);
    return 0;
}

/**
 * @brief Libera el espacio reservado del char device
 */
static void __exit module_kernel_exit(void) {
    device_destroy(chrdev_class, chrdev_number);
    class_destroy(chrdev_class);
    unregister_chrdev_region(chrdev_number, CHRDEV_COUNT);
    cdev_del(&chrdev);
    serdev_device_driver_unregister(&egb_uart_driver);
    printk(KERN_INFO "%s: Modulo removido\n", AUTHOR);
}

// Funciones de inicializacion y salida
module_init(module_kernel_init);
module_exit(module_kernel_exit);

// Informacion del modulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("Modulo de kernel EGB");