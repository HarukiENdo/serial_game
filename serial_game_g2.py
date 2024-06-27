import pygame #ゲーム作成用ライブラリ
import random
import serial #シリアル通信用
import threading
import sys
import time

# シリアル通信用グローバル変数
part1, part2, part3 = "0", "0", "7"
serial_lock = threading.Lock() #同期プログラムと競合が発生しないようにするため

# シリアル通信の関数
def serial_communication():
    global part1, part2, part3  # dataをグローバル変数として宣言
    COM = '/dev/ttyACM0' #COMポートの設定
    bitRate = 38400 #通信速度
    try:
        ser = serial.Serial(COM, bitRate, timeout=0.1)
        while True:
            ser.write(part3.encode()) #LED状態の書き込み
            received_data = ser.read_all() #受信データの読み込み
            if received_data:
                serial_lock.acquire()
                try:
                    if len(received_data) == 8:
                        r_part1 = received_data[:4].decode() #ロータリエンコーダの状態
                        r_part2 = received_data[4:6].decode() #SWの状態
                        part1, part2 = r_part1, r_part2
                finally:
                    serial_lock.release()
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit()

# 定数設定
GAME_OVER_SCORE = 8
WIDTH = 800
HEIGHT = 600
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
YELLOW = (255, 255, 0)
WIN_SCORE = 8

def show_start_screen(screen, font): #スタート画面処理関数
    global part1, part2, part3
    part1, part2 = "FF0F", "aa"
    background_image = pygame.image.load('start.jpg').convert_alpha()
    background_image = pygame.transform.scale(background_image, (WIDTH, HEIGHT))
    start_text = font.render('PRESS SW2 TO START', True, WHITE)
    start_text_rect = start_text.get_rect(center=(WIDTH // 2, HEIGHT // 2))
    screen.blit(background_image, (0, 0))
    screen.blit(start_text, start_text_rect)
    pygame.display.flip()

    running = False
    while not running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif part2 == "an":
                running = True

def gameover(screen, font): #ゲームオーバー処理関数
    global part1, part2, part3
    part1, part2 = "FF0F", "aa"
    background_image = pygame.image.load('game_over.png').convert_alpha()
    background_image = pygame.transform.scale(background_image, (WIDTH, HEIGHT))
    screen.blit(background_image, (0, 0))
    game_over_text = font.render('GAME OVER (T-T)', True, WHITE)
    text_rect = game_over_text.get_rect(center=(WIDTH / 2, HEIGHT / 2))
    screen.blit(game_over_text, text_rect)
    pygame.display.flip()

    running = False
    while not running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif part2 == "an":
                running = True
                main()

def win(screen, font): #win画面処理関数
    global part1, part2, part3
    part1, part2 = "FF0F", "aa"
    background_image = pygame.image.load('win.png').convert_alpha()
    background_image = pygame.transform.scale(background_image, (WIDTH, HEIGHT))
    screen.blit(background_image, (0, 0))
    game_over_text = font.render('You win!', True, WHITE)
    text_rect = game_over_text.get_rect(center=(WIDTH / 2, HEIGHT / 2))
    screen.blit(game_over_text, text_rect)
    pygame.display.flip()

    running = False
    while not running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif part2 == "an":
                running = True
                main()

def pause(screen, font): #ポーズ画面処理関数
    background_image = pygame.image.load('pause.png').convert_alpha()
    background_image = pygame.transform.scale(background_image, (WIDTH, HEIGHT))
    pause_text = font.render('PAUSED  PRESS SW2 TO CONTINUE', True, WHITE)
    pause_text_rect = pause_text.get_rect(center=(WIDTH // 2, HEIGHT // 2))
    screen.blit(background_image, (0, 0))
    screen.blit(pause_text, pause_text_rect)
    pygame.display.flip()

    paused = True
    while paused:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
        serial_lock.acquire()
        try:
            if part2 == "an":
                paused = False
        finally:
            serial_lock.release()

def main(): #ゲームのメイン部分
    pygame.init()
    global part1, part2, part3  # dataをグローバル変数として宣言
    part1, part2 = "FF0F", "aa"  # 初期値の設定

    tmp_rot = int(part1[-2:], 16) #ロータリエンコーダ

    width, height = WIDTH, HEIGHT
    screen = pygame.display.set_mode((width, height))
    clock = pygame.time.Clock()
    font = pygame.font.Font(None, 36)

    show_start_screen(screen, font) #スタート画面の設定

    background_image = pygame.image.load('maxresdefault.jpg').convert() #ゲーム画面
    ufo_image = pygame.image.load('UFO.png').convert_alpha() #対象物
    ufo_image = pygame.transform.scale(ufo_image, (100, 50))
    ufo_rect = ufo_image.get_rect(center=(width // 2, 100))
    #ゲーム変数初期化
    ufo = ufo_rect
    ufo_hit_count = 0
    bottom_hit_count = 0
    cnt = 0
    ufo_speed_x = 2
    ufo_direction_x = 1
    ufo_speed_y = 2
    ufo_direction_y = 1

    ball_speed_available = list(range(-5,-3))+list(range(3,5))
    ball_speed_x = random.choice(ball_speed_available)
    ball_speed_y = random.choice(ball_speed_available)
    ball_speed = [ball_speed_x, ball_speed_y]
    ball = pygame.Rect(width // 2 - 15, height // 2 - 15, 15, 15)
    pygame.draw.ellipse(screen, WHITE, ball)
    bar = pygame.Rect(width // 2 - 60, height - 10, 120, 20)
    pygame.draw.rect(screen, YELLOW, bar)
    hit_text = font.render(f'Hits: {ufo_hit_count}', True, YELLOW)
    miss_text = font.render(f'damage: {bottom_hit_count}', True, YELLOW)
    screen.blit(hit_text, (10, 10)) 
    screen.blit(miss_text, (10, 20))

    #ゲームの状態変数
    running = True
    pause_game = False
    game_over = False
    game_win = False

    #runningループ
    while running:
        for event in pygame.event.get(): #右上端の×ボタンで終了
            if event.type == pygame.QUIT:
                running = False
        serial_lock.acquire()
        try:
            if part2 == "na": #sw1 ON
                pause_game = True
            elif part2 == "an": #sw2 ON
                pause_game = False
        finally:
            serial_lock.release()

        cnt += 1
        serial_lock.acquire() #データの競合が発生しないように
        try:
            rot = int(part1[-2:], 16)
        finally:
            serial_lock.release()

        # bar.y = max(0, min(bar.y, height - bar.height))

        if not game_over and not game_win and not pause_game:
            #ボールの動作
            ball.x += ball_speed[0]
            ball.y += ball_speed[1]

            #UFOの動作
            ufo_rect.x += ufo_speed_x * ufo_direction_x
            if ufo_rect.right >= width or ufo_rect.left <= 0:
                ufo_direction_x *= -1

            ufo_rect.y += ufo_speed_y * ufo_direction_y
            if ufo_rect.top <= 0 or ufo_rect.bottom >= height - 100:
                ufo_direction_y *= -1

            #バーの動作
            if tmp_rot > rot:
                bar.x -= 15
                tmp_rot = rot
            elif tmp_rot < rot:
                bar.x += 15
                tmp_rot = rot
            else:
                tmp_rot = rot

            #跳ね返り処理
            if ball.colliderect(bar):
                ball_speed[1] = -ball_speed[1]

            if ball.top <= 0:
                ball_speed[1] = -ball_speed[1]
            if ball.left <= 0 or ball.right >= width:
                ball_speed[0] = -ball_speed[0]
            if ball.bottom >= height:
                ball_speed[1] = -ball_speed[1]
                ball.y = height - ball.height
                bottom_hit_count += 1
                print(f"damage: {bottom_hit_count}") #damageのカウント
                if bottom_hit_count >= GAME_OVER_SCORE:
                    game_over = True

            if ball.colliderect(ufo):
                ball_speed[1] = -ball_speed[1] * 1.5
                ufo_hit_count += 1
                print(f"Hit count: {ufo_hit_count}") #HIT数のカウント
                if ufo_hit_count >= WIN_SCORE:
                    print("You win!")
                    game_win = True

            #画面の更新
            screen.blit(background_image, (0, 0))
            hit_text = font.render(f'Hits: {ufo_hit_count}', True, YELLOW)
            miss_text = font.render(f'damage: {bottom_hit_count}', True, YELLOW)
            screen.blit(hit_text, (10, 10))
            screen.blit(miss_text, (10, 30))
            pygame.draw.rect(screen, WHITE, bar)
            pygame.draw.ellipse(screen, YELLOW, ball)
            screen.blit(ufo_image, ufo_rect)
            pygame.display.flip()
            part3 = str(GAME_OVER_SCORE - bottom_hit_count)
        else:
            if game_win: #勝利画面に遷移
                win(screen, font)
            elif game_over: #敗北画面に遷移
                gameover(screen, font)
            elif pause_game: #ポーズ画面に遷移
                pause(screen, font)

        clock.tick(60)

    pygame.quit() #システムの終了
    sys.exit()

def check_serial_input(): #通信ができているかの確認関数
    global part1, part2
    while True:
        time.sleep(0.1)
        print(part1, part2)
        # if part2 == "an":

def main_thread(): #serial通信とゲームmain部分を別スレッドで実行　同期的に処理を行う
    global part1, part2
    part1, part2 = "aa", "aa"

    serial_thread = threading.Thread(target=serial_communication)
    serial_thread.start()

    # check_serial_input()
    while True:
        time.sleep(0.1)
        main()

main_thread()
