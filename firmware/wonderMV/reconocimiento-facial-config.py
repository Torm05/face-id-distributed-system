import sensor, image, time, lcd
import gc
import uos
import struct
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

# ── Funciones de persistencia en SD ─────────────────────────────────────────
def inicializar_directorio():
    try:
        uos.mkdir(FACES_DIR)
        print("[SD] Carpeta creada:", FACES_DIR)
    except:
        print("[SD] Carpeta ya existe:", FACES_DIR)

def cargar_rostros():
    rostros = []
    try:
        archivos = sorted(uos.listdir(FACES_DIR))
        for archivo in archivos:
            if archivo.endswith(".bin"):
                ruta = FACES_DIR + "/" + archivo
                with open(ruta, "rb") as f:
                    data = f.read()
                n = len(data) // 4
                feature = list(struct.unpack("{}f".format(n), data))
                rostros.append(feature)
                print("[SD] Cargado:", archivo)
    except Exception as e:
        print("[SD] Error cargando rostros:", e)
    return rostros

def guardar_rostro(feature, index):
    ruta = FACES_DIR + "/face_{:03d}.bin".format(index)
    try:
        data = struct.pack("{}f".format(len(feature)), *feature)
        with open(ruta, "wb") as f:
            f.write(data)
        print("[SD] Guardado:", ruta)
    except Exception as e:
        print("[SD] Error guardando:", e)

def borrar_rostro(index):
    ruta = FACES_DIR + "/face_{:03d}.bin".format(index)
    try:
        uos.remove(ruta)
        print("[SD] Borrado:", ruta)
    except Exception as e:
        print("[SD] Error borrando:", e)

# ── Modelos KPU ────────────────────────────────────────────────────────────
feature_img = image.Image(size=(64, 64), copy_to_fb=False)
feature_img.pix_to_ai()

FACE_PIC_SIZE = 64
dst_point = [
    (int(38.2946 * FACE_PIC_SIZE / 112), int(51.6963 * FACE_PIC_SIZE / 112)),
    (int(73.5318 * FACE_PIC_SIZE / 112), int(51.5014 * FACE_PIC_SIZE / 112)),
    (int(56.0252 * FACE_PIC_SIZE / 112), int(71.7366 * FACE_PIC_SIZE / 112)),
    (int(41.5493 * FACE_PIC_SIZE / 112), int(92.3655 * FACE_PIC_SIZE / 112)),
    (int(70.7299 * FACE_PIC_SIZE / 112), int(92.2041 * FACE_PIC_SIZE / 112)),
]

anchor = (0.1075, 0.126875, 0.126875, 0.175, 0.1465625, 0.2246875,
          0.1953125, 0.25375, 0.2440625, 0.351875, 0.341875, 0.4721875,
          0.5078125, 0.6696875, 0.8984375, 1.099687, 2.129062, 2.425937)

kpu = KPU()
kpu.load_kmodel("/sd/KPU/yolo_face_detect/face_detect_320x240.kmodel")
kpu.init_yolo2(anchor, anchor_num=9, img_w=320, img_h=240,
               net_w=320, net_h=240, layer_w=10, layer_h=8,
               threshold=0.7, nms_value=0.2, classes=1)

ld5_kpu = KPU()
ld5_kpu.load_kmodel("/sd/KPU/face_recognization/ld5.kmodel")

fea_kpu = KPU()
fea_kpu.load_kmodel("/sd/KPU/face_recognization/feature_extraction.kmodel")

# ── Bandera para registrar rostros ─────────────────────────────────────────
start_processing = False

# ── Cargar rostros persistidos desde SD ────────────────────────────────────
inicializar_directorio()
record_ftrs = cargar_rostros()
print("[INIT] Rostros cargados desde SD:", len(record_ftrs))

# ── Umbral de reconocimiento ────────────────────────────────────────────────
THRESHOLD = 80.5

# ── Control de envio UART ───────────────────────────────────────────────────
ultimo_id_enviado = -99
ultimo_envio = 0
INTERVALO_REENVIO = 3000
TIMEOUT_SIN_ROSTRO = 2000

# ── Helpers ──────────────────────────────────────────────────────────────────
def extend_box(x, y, w, h, scale):
    x1_t = x - scale * w
    x2_t = x + w + scale * w
    y1_t = y - scale * h
    y2_t = y + h + scale * h
    x1 = int(x1_t) if x1_t > 1 else 1
    x2 = int(x2_t) if x2_t < 320 else 319
    y1 = int(y1_t) if y1_t > 1 else 1
    y2 = int(y2_t) if y2_t < 240 else 239
    return x1, y1, x2 - x1 + 1, y2 - y1 + 1

def enviar_id(face_id):
    global ultimo_id_enviado, ultimo_envio
    ahora = time.ticks_ms()
    if (face_id != ultimo_id_enviado or
            time.ticks_diff(ahora, ultimo_envio) > INTERVALO_REENVIO):
        mensaje_str = "ID:{}\n".format(face_id)
        mensaje_bytes = [ord(c) for c in mensaje_str]
        uart.send_bytearray(mensaje_bytes)
        print("[UART] Enviado:", mensaje_str.strip())
        ultimo_id_enviado = face_id
        ultimo_envio = ahora

# ── Loop principal ────────────────────────────────────────────────────────────
try:
    while True:
        gc.collect()
        clock.tick()

        # ───────── Escuchar comandos por UART ───────────────────────────────
        if uart.any():
            comando = uart.read()
            if comando and b'REG' in comando:
                start_processing = True
                print("[CMD] Solicitud de registro recibida por UART")

        img = sensor.snapshot()

        kpu.run_with_output(img)
        dect = kpu.regionlayer_yolo2()
        fps = clock.fps()
        ahora = time.ticks_ms()

        hay_rostro = len(dect) > 0

        if hay_rostro:
            for l in dect:
                x1, y1, cut_w, cut_h = extend_box(l[0], l[1], l[2], l[3], scale=0)

                face_cut = img.cut(x1, y1, cut_w, cut_h)
                face_cut_128 = face_cut.resize(128, 128)
                face_cut_128.pix_to_ai()

                out = ld5_kpu.run_with_output(face_cut_128, getlist=True)
                face_key_point = []
                for j in range(5):
                    x = int(KPU.sigmoid(out[2 * j]) * cut_w + x1)
                    y = int(KPU.sigmoid(out[2 * j + 1]) * cut_h + y1)
                    face_key_point.append((x, y))

                T = image.get_affine_transform(face_key_point, dst_point)
                image.warp_affine_ai(img, feature_img, T)
                feature = fea_kpu.run_with_output(feature_img, get_feature=True)
                del face_key_point

                # ── Comparar contra registrados ─────────────────────────────
                scores = []
                for j in range(len(record_ftrs)):
                    score = kpu.feature_compare(record_ftrs[j], feature)
                    scores.append(score)

                if scores:
                    max_score = max(scores)
                    index = scores.index(max_score)

                    if max_score > THRESHOLD:
                        # Rostro conocido
                        face_id_servidor = index + 1
                        img.draw_rectangle(l[0], l[1], l[2], l[3],
                                           color=(0, 255, 0))
                        img.draw_string(l[0], l[1] - 18,
                                        "ID:{} {:.0f}%".format(face_id_servidor, max_score),
                                        color=(0, 255, 0), scale=1.5)
                        enviar_id(face_id_servidor)
                    else:
                        # Rostro desconocido
                        img.draw_rectangle(l[0], l[1], l[2], l[3],
                                           color=(255, 0, 0))
                        img.draw_string(l[0], l[1] - 18,
                                        "Desconocido {:.0f}%".format(max_score),
                                        color=(255, 0, 0), scale=1.5)
                        enviar_id(0)
                else:
                    # Sin nadie registrado aun
                    img.draw_rectangle(l[0], l[1], l[2], l[3],
                                       color=(255, 255, 0))
                    img.draw_string(l[0], l[1] - 18,
                                    "Sin registros",
                                    color=(255, 255, 0), scale=1.5)
                    enviar_id(0)

                # ── Registrar rostro mediante Comando UART ──────────────────
                if start_processing:
                    record_ftrs.append(feature)
                    nuevo_id = len(record_ftrs)
                    guardar_rostro(feature, nuevo_id)
                    print("[REG] Rostro registrado y guardado como ID:", nuevo_id)
                    img.draw_string(0, 180,
                                    "Guardado ID:{}".format(nuevo_id),
                                    color=(0, 255, 255), scale=2)
                    lcd.display(img)
                    time.sleep_ms(1500)
                    start_processing = False

                del scores
                del face_cut_128
                del face_cut

        else:
            # Sin rostro por mas de TIMEOUT_SIN_ROSTRO ms
            if (ultimo_id_enviado != -1 and
                    time.ticks_diff(ahora, ultimo_envio) > TIMEOUT_SIN_ROSTRO):
                enviar_id(-1)

        # ── Info en pantalla ────────────────────────────────────────────────
        img.draw_string(0, 0,
                        "{:.1f}fps".format(fps),
                        color=(0, 60, 255), scale=2.0)
        img.draw_string(0, 215,
                        "CMD: REG  IDs:{} ".format(len(record_ftrs)),
                        color=(255, 100, 0), scale=1.5)
        lcd.display(img)

except Exception as e:
    raise e
finally:
    kpu.deinit()
    ld5_kpu.deinit()
    fea_kpu.deinit()
