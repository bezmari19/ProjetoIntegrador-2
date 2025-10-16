#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>

// ===== Enum para os estados do jogo =====
typedef enum {
    MENU_INICIAL,
    JOGO,
    MOSTRAR_REGRAS,
    MOSTRAR_SOBRE
} Estado;

// ===== Estrutura para representar botões =====
typedef struct {
    int x1, y1, x2, y2;
} Botao;

// ===== Função auxiliar: fade out/in =====
void fade(ALLEGRO_DISPLAY* janela, ALLEGRO_BITMAP* fundo, int modeWidth, int modeHeight, bool fadeOut) {
    int passo = 5;
    int start = fadeOut ? 0 : 255;
    int end = fadeOut ? 255 : 0;

    if (fadeOut) {
        for (int alpha = start; alpha <= end; alpha += passo) {
            al_draw_scaled_bitmap(fundo, 0, 0,
                al_get_bitmap_width(fundo),
                al_get_bitmap_height(fundo),
                0, 0, modeWidth, modeHeight, 0);
            al_draw_filled_rectangle(0, 0, modeWidth, modeHeight, al_map_rgba(0, 0, 0, alpha));
            al_flip_display();
            al_rest(0.01);
        }
    }
    else {
        for (int alpha = start; alpha >= end; alpha -= passo) {
            al_draw_scaled_bitmap(fundo, 0, 0,
                al_get_bitmap_width(fundo),
                al_get_bitmap_height(fundo),
                0, 0, modeWidth, modeHeight, 0);
            al_draw_filled_rectangle(0, 0, modeWidth, modeHeight, al_map_rgba(0, 0, 0, alpha));
            al_flip_display();
            al_rest(0.01);
        }
    }
}

int main(void)
{
    // ===== Inicializações =====
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();

    ALLEGRO_DISPLAY_MODE mode;
    al_get_display_mode(al_get_num_display_modes() - 1, &mode);
    ALLEGRO_DISPLAY* janela = al_create_display(mode.width, mode.height);
    al_set_window_title(janela, "Personagem e Cenarios");

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(janela));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());

    // ===== Sprites do personagem principal =====
    ALLEGRO_BITMAP* img_dir = al_load_bitmap("andando_direita.png");
    ALLEGRO_BITMAP* img_esq = al_load_bitmap("andando_esquerda.png");
    ALLEGRO_BITMAP* img_cima = al_load_bitmap("andando_cima.png");
    ALLEGRO_BITMAP* img_baixo = al_load_bitmap("andando_baixo.png");
    ALLEGRO_BITMAP* imagem = img_dir;

    // ===== NPC / Instrutor =====
    ALLEGRO_BITMAP* img_npc = al_load_bitmap("npc_instrutor.png");
    float npc_x = 600;
    float npc_y = 400;
    bool npc_visivel = false;
    float npc_timer = 0;
    float npc_duracao = 5.0;
    bool npc_falando = true;

    // ===== Cenários =====
    ALLEGRO_BITMAP* cenarios[3] = { NULL, NULL, NULL };
    cenarios[0] = al_load_bitmap("cenario1.png");
    cenarios[1] = al_load_bitmap("cenario2.png");
    cenarios[2] = al_load_bitmap("cenario3.png");

    // ===== Fonte e cores =====
    ALLEGRO_FONT* font = al_load_ttf_font("arial.ttf", 28, 0);
    ALLEGRO_COLOR branco = al_map_rgb(255, 255, 255);

    // ===== Estado e variáveis =====
    Estado estado = MENU_INICIAL;
    bool running = true;
    float x = 0, y = 0;
    int cenario_atual = 0;

    // ===== Botões (não desenham nada, apenas para clique) =====
    Botao botaoJogar = { 500, 400, 800, 480 };
    Botao botaoRegras = { 500, 500, 800, 580 };
    Botao botaoSobre = { 500, 600, 800, 680 };

    float scaleX = (float)mode.width / al_get_bitmap_width(cenarios[0]);
    float scaleY = (float)mode.height / al_get_bitmap_height(cenarios[0]);

    ALLEGRO_KEYBOARD_STATE keyState;

    al_start_timer(timer);

    // ===== Loop principal =====
    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            running = false;

        // ===== Clique do mouse =====
        if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = event.mouse.x;
            int my = event.mouse.y;
            int mx_scaled = mx / scaleX;
            int my_scaled = my / scaleY;

            if (estado == MENU_INICIAL) {
                if (mx_scaled >= botaoJogar.x1 && mx_scaled <= botaoJogar.x2 &&
                    my_scaled >= botaoJogar.y1 && my_scaled <= botaoJogar.y2) {

                    al_stop_timer(timer);
                    fade(janela, cenarios[0], mode.width, mode.height, true);

                    cenario_atual = 1;
                    imagem = img_dir;
                    x = 100;
                    y = mode.height - al_get_bitmap_height(imagem) - 50;

                    // Ativa NPC quando entra no jogo
                    npc_visivel = true;
                    npc_timer = 0;

                    fade(janela, cenarios[cenario_atual], mode.width, mode.height, false);
                    estado = JOGO;
                    al_start_timer(timer);
                }
                else if (mx_scaled >= botaoRegras.x1 && mx_scaled <= botaoRegras.x2 &&
                    my_scaled >= botaoRegras.y1 && my_scaled <= botaoRegras.y2) {
                    al_stop_timer(timer);
                    fade(janela, cenarios[0], mode.width, mode.height, true);
                    estado = MOSTRAR_REGRAS;
                    fade(janela, cenarios[0], mode.width, mode.height, false);
                    al_start_timer(timer);
                }
                else if (mx_scaled >= botaoSobre.x1 && mx_scaled <= botaoSobre.x2 &&
                    my_scaled >= botaoSobre.y1 && my_scaled <= botaoSobre.y2) {
                    al_stop_timer(timer);
                    fade(janela, cenarios[0], mode.width, mode.height, true);
                    estado = MOSTRAR_SOBRE;
                    fade(janela, cenarios[0], mode.width, mode.height, false);
                    al_start_timer(timer);
                }
            }
        }

        // ===== Atualização a cada frame =====
        if (event.type == ALLEGRO_EVENT_TIMER) {
            al_get_keyboard_state(&keyState);

            // ESC volta ao menu
            if (al_key_down(&keyState, ALLEGRO_KEY_ESCAPE))
                estado = MENU_INICIAL;

            // Movimento do personagem
            if (estado == JOGO) {
                if (al_key_down(&keyState, ALLEGRO_KEY_RIGHT)) { x += (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10; imagem = img_dir; }
                if (al_key_down(&keyState, ALLEGRO_KEY_LEFT)) { x -= (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10; imagem = img_esq; }
                if (al_key_down(&keyState, ALLEGRO_KEY_DOWN)) { y += (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10; imagem = img_baixo; }
                if (al_key_down(&keyState, ALLEGRO_KEY_UP)) { y -= (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10; imagem = img_cima; }

                // Troca de cenário
                int width = al_get_bitmap_width(imagem);
                int height = al_get_bitmap_height(imagem);

                if (x > mode.width) { cenario_atual = (cenario_atual + 1) % 3; x = 0; }
                if (x + width < 0) { cenario_atual = (cenario_atual - 1 + 3) % 3; x = mode.width - width; }
                if (y < 0) y = 0;
                if (y + height > mode.height) y = mode.height - height;

                // Atualiza NPC
                if (npc_visivel) {
                    npc_timer += 1.0 / 60.0;
                    if (npc_timer > npc_duracao)
                        npc_visivel = false;
                }
            }

            // ===== Desenho da tela =====
            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (estado == MENU_INICIAL) {
                al_draw_scaled_bitmap(cenarios[0], 0, 0,
                    al_get_bitmap_width(cenarios[0]),
                    al_get_bitmap_height(cenarios[0]),
                    0, 0, mode.width, mode.height, 0);
            }
            else if (estado == MOSTRAR_REGRAS) {
                al_clear_to_color(al_map_rgb(40, 0, 0));
                al_draw_text(font, branco, mode.width / 2, 100, ALLEGRO_ALIGN_CENTER, "REGRAS DO JOGO");
                al_draw_text(font, branco, mode.width / 2, 200, ALLEGRO_ALIGN_CENTER, "Use as setas para mover o personagem.");
                al_draw_text(font, branco, mode.width / 2, 250, ALLEGRO_ALIGN_CENTER, "Ao chegar na borda, voce muda de cenario.");
                al_draw_text(font, branco, mode.width / 2, 400, ALLEGRO_ALIGN_CENTER, "Pressione ESC para voltar.");
            }
            else if (estado == MOSTRAR_SOBRE) {
                al_clear_to_color(al_map_rgb(0, 0, 40));
                al_draw_text(font, branco, mode.width / 2, 100, ALLEGRO_ALIGN_CENTER, "SOBRE O JOGO");
                al_draw_text(font, branco, mode.width / 2, 200, ALLEGRO_ALIGN_CENTER, "Jogo desenvolvido para explorar o Allegro 5.");
                al_draw_text(font, branco, mode.width / 2, 250, ALLEGRO_ALIGN_CENTER, "Autor: Flavio Roberto Moranda");
                al_draw_text(font, branco, mode.width / 2, 400, ALLEGRO_ALIGN_CENTER, "Pressione ESC para voltar.");
            }
            else if (estado == JOGO) {
                al_draw_scaled_bitmap(cenarios[cenario_atual], 0, 0,
                    al_get_bitmap_width(cenarios[cenario_atual]),
                    al_get_bitmap_height(cenarios[cenario_atual]),
                    0, 0, mode.width, mode.height, 0);

                // Desenha personagem principal
                al_draw_bitmap(imagem, x, y, 0);

                // Desenha NPC se estiver visível
                if (npc_visivel && npc_falando) {
                    al_draw_bitmap(img_npc, npc_x, npc_y, 0);
                    al_draw_filled_rectangle(npc_x - 20, npc_y - 60, npc_x + 300, npc_y - 10, al_map_rgba(0, 0, 0, 150));
                    al_draw_text(font, branco, npc_x, npc_y - 50, 0, "JANGAL!");
                }
            }

            al_flip_display();
        }
    }

    // ===== Libera recursos =====
    al_destroy_display(janela);
    al_uninstall_keyboard();
    al_uninstall_mouse();
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    al_destroy_bitmap(img_cima);
    al_destroy_bitmap(img_baixo);
    al_destroy_bitmap(img_esq);
    al_destroy_bitmap(img_dir);
    al_destroy_bitmap(img_npc);
    for (int i = 0; i < 3; i++) al_destroy_bitmap(cenarios[i]);
    al_destroy_font(font);

    return 0;
}
