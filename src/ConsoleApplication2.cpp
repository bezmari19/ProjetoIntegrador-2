#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

int main(void)
{
    ALLEGRO_DISPLAY* janela = NULL;
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_BITMAP* img_cima = NULL, * img_baixo = NULL, * img_esq = NULL, * img_dir = NULL;
    ALLEGRO_BITMAP* imagem = NULL;
    ALLEGRO_BITMAP* cenarios[3] = { NULL, NULL, NULL };

    // Inicializações
    al_init();
    al_install_keyboard();
    al_init_image_addon();

    // Obter resolução da tela
    ALLEGRO_DISPLAY_MODE mode;
    al_get_display_mode(al_get_num_display_modes() - 1, &mode);

    // Criar janela
    janela = al_create_display(mode.width, mode.height);
    al_set_window_title(janela, "Personagem e Cenários");

    queue = al_create_event_queue();
    timer = al_create_timer(1.0 / 60);

    // Registra eventos
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(janela));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    // Carrega sprites do personagem
    img_dir = al_load_bitmap("andando_direita.png");
    img_esq = al_load_bitmap("andando_esquerda.png");
    img_cima = al_load_bitmap("andando_cima.png");
    img_baixo = al_load_bitmap("andando_baixo.png");
    imagem = img_dir;

    // Carrega cenários
    cenarios[0] = al_load_bitmap("cenario1.png");
    cenarios[1] = al_load_bitmap("cenario2.png");
    cenarios[2] = al_load_bitmap("cenario3.png");

    bool running = true;
    float x = mode.width / 2;
    float y = mode.height / 2;
    int cenario_atual = 0;

    al_start_timer(timer);

    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            running = false;

        // Estado do teclado
        ALLEGRO_KEYBOARD_STATE keyState;
        al_get_keyboard_state(&keyState);

        // Movimento horizontal
        if (al_key_down(&keyState, ALLEGRO_KEY_RIGHT)) {
            x += (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10;
            imagem = img_dir;
        }
        if (al_key_down(&keyState, ALLEGRO_KEY_LEFT)) {
            x -= (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10;
            imagem = img_esq;
        }

        // Movimento vertical
        if (al_key_down(&keyState, ALLEGRO_KEY_DOWN)) {
            y += (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10;
            imagem = img_baixo;
        }
        if (al_key_down(&keyState, ALLEGRO_KEY_UP)) {
            y -= (al_key_down(&keyState, ALLEGRO_KEY_LCTRL)) ? 20 : 10;
            imagem = img_cima;
        }

        // Atualização da tela
        if (event.type == ALLEGRO_EVENT_TIMER) {
            al_clear_to_color(al_map_rgb(0, 0, 0));

            // Desenha o cenário atual
            al_draw_scaled_bitmap(cenarios[cenario_atual], 0, 0,
                al_get_bitmap_width(cenarios[cenario_atual]),
                al_get_bitmap_height(cenarios[cenario_atual]),
                0, 0, mode.width, mode.height, 0);

            // Desenha o personagem
            al_draw_bitmap(imagem, x, y, 0);
            al_flip_display();
        }

        // Limites e troca de cenário
        int width = al_get_bitmap_width(imagem);
        int height = al_get_bitmap_height(imagem);

        // Passa para o próximo cenário
        if (x > mode.width) {
            cenario_atual = (cenario_atual + 1) % 3;
            x = 0;
        }
        // Volta para o cenário anterior
        if (x + width < 0) {
            cenario_atual = (cenario_atual - 1 + 3) % 3;
            x = mode.width - width;
        }

        // Limita verticalmente
        if (y < 0) y = 0;
        if (y + height > mode.height) y = mode.height - height;
    }

    // Libera recursos
    al_destroy_display(janela);
    al_uninstall_keyboard();
    al_destroy_timer(timer);

    al_destroy_bitmap(img_cima);
    al_destroy_bitmap(img_baixo);
    al_destroy_bitmap(img_esq);
    al_destroy_bitmap(img_dir);

    for (int i = 0; i < 3; i++)
        al_destroy_bitmap(cenarios[i]);

    return 0;
}
