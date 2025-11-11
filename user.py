# def user_tx(txt):
#     with open('/dev/lux_control', 'w') as f:
#         print(f"Escribiendo: {txt}")
#         f.write(txt)
#     return

# def user_rx():
#     with open('/dev/lux_control', 'r') as f:
#         txt = f.read()
#         print(f"Leido: {txt}")
#     return txt

# user_tx("Hola Mundo")
# user_rx()
import time

# with open('/dev/lux_control', 'r+') as f:
#     txt = f.read()
#     while True:
#         f.write(input("Ingrese el comando: "))
#         txt = f.read()
#         print(f"Leido: {txt}")
#         time.sleep(1)

while True:
    # cmd = input("lux_control:")
    # with open('/dev/lux_control', 'w') as f:    
    #     f.write(cmd + '\n')
    with open('/dev/lux_control', 'r') as f:
        r = f.read()
        print(f"Lectura: {r}")