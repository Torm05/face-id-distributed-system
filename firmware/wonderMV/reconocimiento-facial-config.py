import sensor, image, time, lcd, gc, uos, struct
from hiwonder import fill_light
from maix import KPU, GPIO, utils
from fpioa_manager import fm
from hiwonder import hw_uart

# ── Inicializar LED y pantalla ──────────────────────────────────────────────
led = fill_light()
lcd.init()

# ── Inicializar camara ──────────────────────────────────────────────────────
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=100)
clock = time.clock()
led.fill_onoff(0)

# ── Inicializar UART hacia Arduino UNO ─────────────────────────────────────
uart = hw_uart()
uart.init(9600)

# ── Directorio de rostros en SD ─────────────────────────────────────────────
FACES_DIR = "/sd/faces"

# ── Funciones de persistencia en SD y Logs ──────────────────────────────────
def enviar_log(mensaje):
    print(mensaje)
    texto = "LOG:{}\n".format(mensaje)
    uart.send_bytearray([ord(c) for c in texto])

def enviar_comando_numerico(numero):
    texto = "{}\n".format(numero)
    uart.send_bytearray([ord(c) for c in texto])

def inicializar_directorio():
    try:
        uos.mkdir(FACES_DIR)
        enviar_log("[SD] Carpeta creada")
    except:
        pass

def cargar_rostros():
    rostros = []
    try:
        archivos = sorted(uos.listdir(FACES_DIR))
        max_id = 0
        for archivo in archivos:
            if archivo.startswith("face_") and archivo.endswith(".bin"):
                id_num = int(archivo.replace("face_", "").replace(".bin", ""))
                if id_num > max_id: max_id = id_num
        rostros = [None] * max_id
        for archivo in archivos:
            if archivo.startswith("face_") and archivo.endswith(".bin"):
                id_num = int(archivo.replace("face_", "").replace(".bin", ""))
                ruta = FACES_DIR + "/" + archivo
                with open(ruta, "rb") as f:
                    data = f.read()
                n = len(data) // 4
                feature = list(struct.unpack("{}f".format(n), data))
                rostros[id_num - 1] = feature
    except Exception as e:
        enviar_log("[SD] Error cargando: " + str(e))
    return rostros

def guardar_rostro(feature, index):
    ruta = FACES_DIR + "/face_{:03d}.bin".format(index)
    try:
        data = struct.pack("{}f".format(len(feature)), *feature)
        with open(ruta, "wb") as f:
            f.write(data)
    except Exception as e:
        enviar_log("[SD] Error guardando: " + str(e))

def borrar_rostro(index):
    ruta = FACES_DIR + "/face_{:03d}.bin".format(index)
    try:
        uos.remove(ruta)
    except Exception as e:
        enviar_log("[SD] Error borrando: " + str(e))

# ── Modelos KPU ────────────────────────────────────────────────────────────
feature_img = image.Image(size=(64, 64), copy_to_fb=False)
feature_img.pix_to_ai()
FACE_PIC_SIZE = 64
dst_point = [(int(38.2946 * FACE_PIC_SIZE / 112), int(51.6963 * FACE_PIC_SIZE / 112)),
             (int(73.5318 * FACE_PIC_SIZE / 112), int(51.5014 * FACE_PIC_SIZE / 112)),
             (int(56.0252 * FACE_PIC_SIZE / 112), int(71.7366 * FACE_PIC_SIZE / 112)),
             (int(41.5493 * FACE_PIC_SIZE / 112), int(92.3655 * FACE_PIC_SIZE / 112)),
             (int(70.7299 * FACE_PIC_SIZE / 112), int(92.2041 * FACE_PIC_SIZE / 112))]
anchor = (0.1075, 0.126875, 0.126875, 0.175, 0.1465625, 0.2246875,
          0.1953125, 0.25375, 0.2440625, 0.351875, 0.341875, 0.4721875,
          0.5078125, 0.6696875, 0.8984375, 1.099687, 2.129062, 2.425937)

kpu = KPU()
kpu.load_kmodel("/sd/KPU/yolo_face_detect/face_detect_320x240.kmodel")
kpu.init_yolo2(anchor, anchor_num=9, img_w=320, img_h=240, net_w=320, net_h=240, layer_w=10, layer_h=8, threshold=0.7, nms_value=0.2, classes=1)
ld5_kpu = KPU()
ld5_kpu.load_kmodel("/sd/KPU/face_recognization/ld5.kmodel")
fea_kpu = KPU()
fea_kpu.load_kmodel("/sd/KPU/face_recognization/feature_extraction.kmodel")

# ── Variables de Estado ────────────────────────────────────────────────────
start_processing = False
delete_feedback = ""
feedback_timer = 0
THRESHOLD = 80.5
ultimo_id_enviado = -99
ultimo_envio = 0
INTERVALO_REENVIO = 3000
TIMEOUT_SIN_ROSTRO = 2000

# ── Cargar rostros persistidos desde SD ────────────────────────────────────
inicializar_directorio()
record_ftrs = cargar_rostros()

# ── Helpers ──────────────────────────────────────────────────────────────────
def extend_box(x, y, w, h, scale):
    x1_t = x - scale * w; x2_t = x + w + scale * w; y1_t = y - scale * h; y2_t = y + h + scale * h
    x1 = int(x1_t) if x1_t > 1 else 1; x2 = int(x2_t) if x2_t < 320 else 319
    y1 = int(y1_t) if y1_t > 1 else 1; y2 = int(y2_t) if y2_t < 240 else 239
    return x1, y1, x2 - x1 + 1, y2 - y1 + 1

def procesar_envio_id(face_id):
    global ultimo_id_enviado, ultimo_envio
    ahora = time.ticks_ms()
    if (face_id != ultimo_id_enviado or time.ticks_diff(ahora, ultimo_envio) > INTERVALO_REENVIO):
        enviar_comando_numerico(face_id)
        ultimo_id_enviado = face_id
        ultimo_envio = ahora

# ── Loop principal ────────────────────────────────────────────────────────────
try:
    while True:
        gc.collect()
        clock.tick()
        ahora = time.ticks_ms()

        # ── INTERPRETE DE COMANDOS UART ───────────────────────────────────────────
        if uart.any():
            comando_bytes = uart.read()
            if comando_bytes:
                try:
                    comando_num = int(comando_bytes.decode('utf-8').strip())

                    if comando_num == 1000: # Comando: Registrar Nuevo Rostro
                        start_processing = True
                        enviar_log("[CMD] Comando de Registro recibido")

                    elif comando_num == 3000: # Comando: Borrar Todo
                        record_ftrs.clear()
                        for archivo in uos.listdir(FACES_DIR):
                            if archivo.endswith(".bin"): uos.remove(FACES_DIR + "/" + archivo)
                        delete_feedback = "Todo Borrado"
                        feedback_timer = ahora
                        enviar_comando_numerico(3000) # Respuesta: 3000 (Borrado OK)
                        enviar_log("[CMD] Comando DELALL ejecutado")

                    elif 2000 < comando_num < 3000: # Comando: Borrar ID Especifico (Ej: 2005 -> Borra ID 5)
                        id_a_borrar = comando_num - 2000
                        if 1 <= id_a_borrar <= len(record_ftrs):
                            record_ftrs[id_a_borrar - 1] = None
                            borrar_rostro(id_a_borrar)
                            delete_feedback = "ID:{} Borrado".format(id_a_borrar)
                            feedback_timer = ahora
                            enviar_comando_numerico(comando_num) # Respuesta: El mismo código como OK (Ej: 2005)
                            enviar_log("[CMD] ID {} eliminado".format(id_a_borrar))
                except Exception as e:
                    pass # Ignora ruido

        img = sensor.snapshot()
        kpu.run_with_output(img)
        dect = kpu.regionlayer_yolo2()
        fps = clock.fps()

        if len(dect) > 0:
            for l in dect:
                x1, y1, cut_w, cut_h = extend_box(l[0], l[1], l[2], l[3], scale=0)
                face_cut = img.cut(x1, y1, cut_w, cut_h)
                face_cut_128 = face_cut.resize(128, 128)
                face_cut_128.pix_to_ai()
                out = ld5_kpu.run_with_output(face_cut_128, getlist=True)
                face_key_point = []
                for j in range(5):
                    face_key_point.append((int(KPU.sigmoid(out[2 * j]) * cut_w + x1), int(KPU.sigmoid(out[2 * j + 1]) * cut_h + y1)))
                T = image.get_affine_transform(face_key_point, dst_point)
                image.warp_affine_ai(img, feature_img, T)
                feature = fea_kpu.run_with_output(feature_img, get_feature=True)

                # ── Comparar contra registrados ─────────────────────────────────────────────
                scores = [0.0 if f is None else kpu.feature_compare(f, feature) for f in record_ftrs]

                if scores:
                    max_score = max(scores)
                    index = scores.index(max_score)
                    if max_score > THRESHOLD:
                        face_id_servidor = index + 1
                        img.draw_rectangle(l[0], l[1], l[2], l[3], color=(0, 255, 0))
                        procesar_envio_id(face_id_servidor) # Envia: 1 a 999 (Rostro Conocido)
                    else:
                        img.draw_rectangle(l[0], l[1], l[2], l[3], color=(255, 0, 0))
                        procesar_envio_id(0) # Envia: 0 (Desconocido)
                else:
                    img.draw_rectangle(l[0], l[1], l[2], l[3], color=(255, 255, 0))
                    procesar_envio_id(0) # Envia: 0 (Desconocido/Sin registros)

                # ── Registrar rostro mediante Comando UART ──────────────────────────────────
                if start_processing:
                    try: nuevo_id = record_ftrs.index(None) + 1; record_ftrs[nuevo_id - 1] = feature
                    except ValueError: record_ftrs.append(feature); nuevo_id = len(record_ftrs)
                    guardar_rostro(feature, nuevo_id)
                    delete_feedback = "Guardado ID:{}".format(nuevo_id)
                    feedback_timer = ahora
                    start_processing = False
                    enviar_comando_numerico(1000 + nuevo_id) # Respuesta: 100X (Ej: 1004 = ID 4 guardado OK)
                    enviar_log("[PROCESO] Guardado ID:{}".format(nuevo_id))

        else:
            if (ultimo_id_enviado != -1 and time.ticks_diff(ahora, ultimo_envio) > TIMEOUT_SIN_ROSTRO):
                procesar_envio_id(-1) # Envia: -1 (Sin rostro en cámara)

        # ── Info en pantalla ────────────────────────────────────────────────────────
        if time.ticks_diff(ahora, feedback_timer) < 2000:
            img.draw_string(0, 180, delete_feedback, color=(0, 255, 255), scale=2)

        lcd.display(img)

except Exception as e:
    raise e
finally:
    kpu.deinit(); ld5_kpu.deinit(); fea_kpu.deinit()
