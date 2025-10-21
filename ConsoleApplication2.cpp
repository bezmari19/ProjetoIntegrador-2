// ======= main.c =======
#define _CRT_SECURE_NO_WARNINGS
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> // strcpy, sprintf

// ======= ENUMS E ESTRUTURAS =======
typedef enum {
    MENU_INICIAL,
    JOGO,
    MOSTRAR_REGRAS,
    MOSTRAR_SOBRE,
    GAME_OVER
} Estado;

typedef struct { int x1, y1, x2, y2; } Botao;
typedef struct { float x, y, w, h; } Rect;

// ======= FUNÇÕES AUXILIARES =======
bool colidiu(Rect a, Rect b) {
    return (a.x < b.x + b.w) &&
        (a.x + a.w > b.x) &&
        (a.y < b.y + b.h) &&
        (a.y + a.h > b.y);
}

void fade(ALLEGRO_DISPLAY* janela, ALLEGRO_BITMAP* fundo, int w, int h, bool fadeOut) {
    int passo = 5;
    int start = fadeOut ? 0 : 255;
    int end = fadeOut ? 255 : 0;

    for (int alpha = start; fadeOut ? (alpha <= end) : (alpha >= end); alpha += (fadeOut ? passo : -passo)) {
        if (fundo) {
            al_draw_scaled_bitmap(fundo, 0, 0, al_get_bitmap_width(fundo), al_get_bitmap_height(fundo), 0, 0, w, h, 0);
        }
        al_draw_filled_rectangle(0, 0, w, h, al_map_rgba(0, 0, 0, alpha));
        al_flip_display();
        al_rest(0.01);
    }
}

// ======= VARIÁVEIS GLOBAIS =======
int pontuacao = 0;
int vidas = 3;

int main(void) {
    // ===== Inicializações =====
    if (!al_init()) { printf("Falha ao iniciar Allegro\n"); return -1; }
    al_install_keyboard();
    al_install_mouse();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_install_audio();
    al_init_acodec_addon();
    al_reserve_samples(8);

    ALLEGRO_DISPLAY_MODE mode;
    al_get_display_mode(al_get_num_display_modes() - 1, &mode);
    ALLEGRO_DISPLAY* janela = al_create_display(mode.width, mode.height);
    if (!janela) { printf("Falha ao criar janela\n"); return -1; }
    al_set_window_title(janela, "Jogo com Vidas e Pontuação");

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(janela));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());

    // ===== Sprites =====
    ALLEGRO_BITMAP* img_dir = al_load_bitmap("andando_direita.png");
    ALLEGRO_BITMAP* img_esq = al_load_bitmap("andando_esquerda.png");
    ALLEGRO_BITMAP* img_cima = al_load_bitmap("andando_cima.png");
    ALLEGRO_BITMAP* img_baixo = al_load_bitmap("andando_baixo.png");
    ALLEGRO_BITMAP* img_npc = al_load_bitmap("npc_instrutor.png"); // nome informado
    ALLEGRO_BITMAP* imagem = img_dir;

    // Avisos caso imagens não carreguem
    if (!img_dir || !img_esq || !img_cima || !img_baixo) {
        printf("Aviso: algum sprite do personagem nao carregou corretamente.\n");
    }
    if (!img_npc) {
        printf("Aviso: imagem do NPC 'pnc_instrutor.png' nao carregou.\n");
    }

    // ===== NPC (controle) =====
    float npc_x = 600, npc_y = 400;
    bool npc_visivel = false;            // inicia invisível - só aparece após resposta
    bool npc_falando = false;
    bool npc_som_tocado = false;
    float npc_timer = 0;
    float npc_duracao = 3.0f;            // tempo em segundos que o NPC fica visível
    char npc_mensagem[128] = "Olá! Responda o desafio!";

    // ===== Cenários =====
    ALLEGRO_BITMAP* cenarios[3] = {
        al_load_bitmap("cenario1.png"),
        al_load_bitmap("cenario2.png"),
        al_load_bitmap("cenario3.png")
    };
    ALLEGRO_BITMAP* cenario_fechado = al_load_bitmap("cenario31.png");
    ALLEGRO_BITMAP* cenario_aberto = al_load_bitmap("cenario32.png");
    ALLEGRO_BITMAP* cenario_desafio_atual = cenario_fechado;

    // ===== Fonte e cores =====
    ALLEGRO_FONT* font = al_load_ttf_font("arial.ttf", 28, 0);
    if (!font) { printf("Erro ao carregar fonte arial.ttf\n"); }
    ALLEGRO_COLOR branco = al_map_rgb(255, 255, 255);
    ALLEGRO_COLOR vermelho = al_map_rgb(255, 0, 0);

    // ===== Estados =====
    Estado estado = MENU_INICIAL;
    bool running = true;
    // posição inicial do personagem: posicionada para ficar no chão do cenário
    float x = 100, y = mode.height - 200;
    int fase_atual = 0;
    bool desafio_ativo = false, desafio_concluido = false;
    Rect obstaculo_desafio = { 400,600, 70, 350 }; // (x, y, w, h) — ajuste se necessário
    bool resposta_certa = false;
    bool mostrar_pergunta = false;

    // ===== Botões =====
    Botao botaoJogar = { 500, 400, 800, 480 };
    Botao botaoRegras = { 500, 500, 800, 580 };
    Botao botaoSobre = { 500, 600, 800, 680 };

    float scaleX = 1.0f, scaleY = 1.0f;
    if (cenarios[0]) {
        scaleX = (float)mode.width / al_get_bitmap_width(cenarios[0]);
        scaleY = (float)mode.height / al_get_bitmap_height(cenarios[0]);
    }

    ALLEGRO_KEYBOARD_STATE keyState;
    int last_key = -1;

    // ===== Sons =====
    ALLEGRO_SAMPLE* som_click = al_load_sample("click.wav");
    ALLEGRO_SAMPLE* som_fala_npc = al_load_sample("npc_fala.wav");
    ALLEGRO_SAMPLE* musica_fundo = al_load_sample("musica_fundo.wav");
    ALLEGRO_SAMPLE_INSTANCE* musica_inst = NULL;

    if (musica_fundo) {
        musica_inst = al_create_sample_instance(musica_fundo);
        if (musica_inst) {
            al_set_sample_instance_playmode(musica_inst, ALLEGRO_PLAYMODE_LOOP);
            al_attach_sample_instance_to_mixer(musica_inst, al_get_default_mixer());
            al_play_sample_instance(musica_inst);
        }
    }

    al_start_timer(timer);

    // ===== Loop principal =====
    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) running = false;
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) last_key = event.keyboard.keycode;
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = event.mouse.x, my = event.mouse.y;
            int mx_scaled = (int)(mx / scaleX), my_scaled = (int)(my / scaleY);

            if (estado == MENU_INICIAL) {
                if (mx_scaled >= botaoJogar.x1 && mx_scaled <= botaoJogar.x2 &&
                    my_scaled >= botaoJogar.y1 && my_scaled <= botaoJogar.y2) {

                    if (som_click) al_play_sample(som_click, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);

                    fade(janela, cenarios[0] ? cenarios[0] : cenario_fechado, mode.width, mode.height, true);
                    estado = JOGO;
                    fase_atual = 1;
                    x = 100;
                    y = mode.height - al_get_bitmap_height(imagem) - 50;
                    desafio_concluido = false;
                    resposta_certa = false;
                    mostrar_pergunta = false;
                    npc_visivel = false; npc_timer = 0; npc_falando = false; npc_som_tocado = false;
                    cenario_desafio_atual = cenario_fechado;
                    pontuacao = 0;
                    vidas = 3;
                    fade(janela, cenarios[fase_atual] ? cenarios[fase_atual] : cenario_fechado, mode.width, mode.height, false);
                }
                else if (mx_scaled >= botaoRegras.x1 && mx_scaled <= botaoRegras.x2 &&
                    my_scaled >= botaoRegras.y1 && my_scaled <= botaoRegras.y2) {
                    estado = MOSTRAR_REGRAS;
                }
                else if (mx_scaled >= botaoSobre.x1 && mx_scaled <= botaoSobre.x2 &&
                    my_scaled >= botaoSobre.y1 && my_scaled <= botaoSobre.y2) {
                    estado = MOSTRAR_SOBRE;
                }
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER) {
            al_get_keyboard_state(&keyState);
            if (al_key_down(&keyState, ALLEGRO_KEY_ESCAPE)) estado = MENU_INICIAL;

            if (estado == JOGO) {
                float dx = 0, dy = 0;
                if (al_key_down(&keyState, ALLEGRO_KEY_RIGHT)) { dx = 10; imagem = img_dir; }
                if (al_key_down(&keyState, ALLEGRO_KEY_LEFT)) { dx = -10; imagem = img_esq; }
                if (al_key_down(&keyState, ALLEGRO_KEY_DOWN)) { dy = 10; imagem = img_baixo; }
                if (al_key_down(&keyState, ALLEGRO_KEY_UP)) { dy = -10; imagem = img_cima; }

                // Verifica futuro com largura/altura do sprite atual
                Rect futuro = { x + dx, y + dy,
                    (float)al_get_bitmap_width(imagem), (float)al_get_bitmap_height(imagem) };

                // Bloqueia movimento caso colida com obstáculo e ainda não respondeu corretamente
                if (colidiu(futuro, obstaculo_desafio) && !resposta_certa) {
                    // separa movimento X e Y para suavizar colisão
                    Rect futuroX = { x + dx, y, (float)al_get_bitmap_width(imagem), (float)al_get_bitmap_height(imagem) };
                    Rect futuroY = { x, y + dy, (float)al_get_bitmap_width(imagem), (float)al_get_bitmap_height(imagem) };

                    if (!colidiu(futuroX, obstaculo_desafio)) x += dx;
                    if (!colidiu(futuroY, obstaculo_desafio)) y += dy;

                    mostrar_pergunta = true;
                }
                else {
                    x += dx; y += dy;
                }

                // limites da tela
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (al_get_bitmap_width(imagem) > 0 && x + al_get_bitmap_width(imagem) > mode.width) x = mode.width - al_get_bitmap_width(imagem);
                if (al_get_bitmap_height(imagem) > 0 && y + al_get_bitmap_height(imagem) > mode.height) y = mode.height - al_get_bitmap_height(imagem);

                // ===== DESAFIO: tratar resposta do usuário =====
                if (mostrar_pergunta) {
                    if (last_key == ALLEGRO_KEY_8) {
                        // Resposta correta
                        resposta_certa = true;
                        mostrar_pergunta = false;
                        cenario_desafio_atual = cenario_aberto;
                        pontuacao += 10;

                        // NPC aparece com mensagem positiva
                        npc_visivel = true;
                        npc_falando = true;
                        npc_timer = 0;
                        npc_som_tocado = false;
                        strcpy(npc_mensagem, "Excelente! Mandou bem!");
                    }
                    else if (last_key >= ALLEGRO_KEY_0 && last_key <= ALLEGRO_KEY_9) {
                        // Resposta incorreta
                        mostrar_pergunta = false;
                        vidas--;

                        // NPC aparece com mensagem de incentivo/correção
                        npc_visivel = true;
                        npc_falando = true;
                        npc_timer = 0;
                        npc_som_tocado = false;
                        strcpy(npc_mensagem, "Hmm... quase! Tente outra vez!");

                        if (vidas <= 0) {
                            estado = GAME_OVER;
                        }
                    }
                    last_key = -1;
                }

                // NPC: atualizar timer e tocar som se necessário
                if (npc_visivel) {
                    npc_timer += 1.0f / 60.0f;
                    if (npc_falando && !npc_som_tocado && som_fala_npc) {
                        al_play_sample(som_fala_npc, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                        npc_som_tocado = true;
                    }
                    if (npc_timer >= npc_duracao) {
                        npc_visivel = false;
                        npc_falando = false;
                        npc_timer = 0;
                    }
                }
            }

            // ===== DESENHO =====
            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (estado == MENU_INICIAL) {
                if (cenarios[0]) {
                    al_draw_scaled_bitmap(cenarios[0], 0, 0,
                        al_get_bitmap_width(cenarios[0]), al_get_bitmap_height(cenarios[0]),
                        0, 0, mode.width, mode.height, 0);
                }
         
            }
            else if (estado == MOSTRAR_REGRAS) {
                al_clear_to_color(al_map_rgb(40, 0, 0));
                al_draw_text(font, branco, mode.width / 2, 100, ALLEGRO_ALIGN_CENTER, "REGRAS DO JOGO");
                al_draw_text(font, branco, mode.width / 2, 200, ALLEGRO_ALIGN_CENTER, "Responda os desafios para ganhar pontos!");
            }
            else if (estado == MOSTRAR_SOBRE) {
                al_clear_to_color(al_map_rgb(0, 0, 40));
                al_draw_text(font, branco, mode.width / 2, 100, ALLEGRO_ALIGN_CENTER, "SOBRE O JOGO");
                al_draw_text(font, branco, mode.width / 2, 200, ALLEGRO_ALIGN_CENTER, "Versão com sistema de vidas e pontuação.");
            }
            else if (estado == GAME_OVER) {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                al_draw_text(font, vermelho, mode.width / 2, mode.height / 2 - 50, ALLEGRO_ALIGN_CENTER, "GAME OVER");
                char score_text[64];
                sprintf(score_text, "Pontuação Final: %d", pontuacao);
                al_draw_text(font, branco, mode.width / 2, mode.height / 2 + 10, ALLEGRO_ALIGN_CENTER, score_text);
                al_draw_text(font, branco, mode.width / 2, mode.height / 2 + 70, ALLEGRO_ALIGN_CENTER, "Pressione ENTER para reiniciar");
                // Reiniciar com ENTER
                if (al_key_down(&keyState, ALLEGRO_KEY_ENTER)) {
                    estado = MENU_INICIAL;
                    pontuacao = 0; vidas = 3; resposta_certa = false; mostrar_pergunta = false;
                }
            }
            else if (estado == JOGO) {
                if (cenarios[fase_atual]) {
                    al_draw_scaled_bitmap(cenarios[fase_atual], 0, 0,
                        al_get_bitmap_width(cenarios[fase_atual]),
                        al_get_bitmap_height(cenarios[fase_atual]),
                        0, 0, mode.width, mode.height, 0);
                }

                if (cenario_desafio_atual) {
                    al_draw_scaled_bitmap(cenario_desafio_atual, 0, 0,
                        al_get_bitmap_width(cenario_desafio_atual),
                        al_get_bitmap_height(cenario_desafio_atual),
                        0, 0, mode.width, mode.height, 0);
                }

                // Desenha obstáculo (apenas visual)
                al_draw_filled_rectangle(
                    obstaculo_desafio.x,
                    obstaculo_desafio.y,
                    obstaculo_desafio.x + obstaculo_desafio.w,
                    obstaculo_desafio.y + obstaculo_desafio.h,
                    al_map_rgba(255, 0, 0, 120)
                );

                // Desenha o personagem
                //if (imagem) al_draw_bitmap(imagem, x, y, 0);

                // Desenha NPC quando visível (com balão e mensagem dinâmica)
                if (npc_visivel && img_npc) {
                    al_draw_bitmap(img_npc, npc_x, npc_y, 0);
                    if (npc_falando) {
                        al_draw_filled_ellipse(npc_x + 140, npc_y - 35, 180, 40, al_map_rgba(0, 0, 0, 150));
                        al_draw_text(font, branco, npc_x + 140, npc_y - 50, ALLEGRO_ALIGN_CENTER, npc_mensagem);
                    }
                }

                if (mostrar_pergunta) {
                    al_draw_filled_rectangle(50, 50, 600, 150, al_map_rgba(0, 0, 0, 180));
                    al_draw_text(font, branco, 80, 70, 0, "Desafio: Quanto é 5 + 3?");
                    al_draw_text(font, branco, 80, 100, 0, "Pressione 8 para correto, outro número para erro.");
                }

                // HUD: Pontuação e Vidas
                char hud[64];
                sprintf(hud, "Pontos: %d   Vidas: %d", pontuacao, vidas);
                al_draw_text(font, branco, 10, 10, 0, hud);

                // Coordenadas do personagem (para ajuste de cenário) - exibidas em branco
                char coords[64];
                sprintf(coords, "X: %.0f  Y: %.0f", x, y);
                al_draw_text(font, branco, 10, 40, 0, coords);
            }

            al_flip_display();
        }
    }

    // ===== Libera Recursos =====
    al_destroy_display(janela);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    if (font) al_destroy_font(font);

    if (img_dir) al_destroy_bitmap(img_dir);
    if (img_esq) al_destroy_bitmap(img_esq);
    if (img_cima) al_destroy_bitmap(img_cima);
    if (img_baixo) al_destroy_bitmap(img_baixo);
    if (img_npc) al_destroy_bitmap(img_npc);

    for (int i = 0; i < 3; ++i) if (cenarios[i]) al_destroy_bitmap(cenarios[i]);
    if (cenario_fechado) al_destroy_bitmap(cenario_fechado);
    if (cenario_aberto) al_destroy_bitmap(cenario_aberto);

    if (som_click) al_destroy_sample(som_click);
    if (som_fala_npc) al_destroy_sample(som_fala_npc);
    if (musica_inst) { al_stop_sample_instance(musica_inst); al_destroy_sample_instance(musica_inst); }
    if (musica_fundo) al_destroy_sample(musica_fundo);

    al_uninstall_keyboard();
    al_uninstall_mouse();

    return 0;
}
