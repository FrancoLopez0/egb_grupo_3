def user_tx(txt):
    with open('/dev/lux_control', 'w') as f:
        print(f"Escribiendo: {txt}")
        f.write(txt)
    return

def user_rx():
    with open('/dev/lux_control', 'r') as f:
        txt = f.read()
        print(f"Leido: {txt}")
    return txt

user_tx("Hola Mundo")
user_rx()