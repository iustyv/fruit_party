#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <stdbool.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <fstream>

ALLEGRO_DISPLAY* display = nullptr;
ALLEGRO_TIMER* timer = nullptr;
ALLEGRO_TIMER* timer_fruit = nullptr;
ALLEGRO_MOUSE_STATE mouse_state;
ALLEGRO_FONT* font_heading = nullptr;
ALLEGRO_FONT* font_start_button = nullptr;
ALLEGRO_FONT* font_options = nullptr;
ALLEGRO_BITMAP* charlie_direction = nullptr;
ALLEGRO_BITMAP* charlie_right = nullptr;
ALLEGRO_BITMAP* charlie_left = nullptr;
ALLEGRO_BITMAP* bfruits = nullptr;
ALLEGRO_BITMAP* heart = nullptr;
ALLEGRO_BITMAP* rotten = nullptr;
ALLEGRO_BITMAP* bonus = nullptr;
ALLEGRO_EVENT event;

float display_width = 0, display_height = 0;
float mouse_x = 0, mouse_y = 0;

bool start_page = true, end_page = false, start_executed = false, back_executed = false, mode_executed = false, changed = false;
bool running = true, game_over = false, game = false, saved = false, srand_init = false, timer_started = false;

int charlie_frame = 0, charlie_height = 40, charlie_width = 73; //wymiary charliego to wymiary jednej klatki
float charlie_x = 0, charlie_y = 0;
int fruit_height = 48, fruit_width = 48, rand_rotten = 1, speed = 3, rand_bonus = 30;
bool fruit_generated = false;
int current = 0, hearts = 3, best = 0;
int font_size = 70, font_button = 40, space_between = 40, score_size = 40, margin = 20;

struct Button {
    int x1;
    int x2;
    int y1;
    int y2;
};

struct Fruit {
    int sx;
    int sy;
    int dx;
    int dy;
    ALLEGRO_BITMAP* fruit_type;
};
Fruit new_fruit;

struct Color {
    int r;
    int g;
    int b;
};

std::vector<Fruit> fruits;

void loadFiles() {

    charlie_left = al_load_bitmap("charlie_left.png");
    charlie_right = al_load_bitmap("charlie_right.png");
    charlie_direction = charlie_right;

    bfruits = al_load_bitmap("fruits.png");
    rotten = al_load_bitmap("rotten.png");

    heart = al_load_bitmap("heart.png");
    bonus = al_load_bitmap("crystal.png");

    font_heading = al_load_ttf_font("ponde___.ttf", 70, 0);
    font_start_button = al_load_ttf_font("ponde___.ttf", 40, 0);
    font_options = al_load_ttf_font("AbaddonBold.ttf", 40, 0);
}

void drawStartScreen(const Color& start, const Color& background, const Color& text, const Color& mode) {

    al_clear_to_color(al_map_rgb(background.r, background.g, background.b));
    al_draw_text(font_heading, al_map_rgb(text.r, text.g, text.b), display_width / 2, (display_height - font_size - space_between - font_button) / 2, ALLEGRO_ALIGN_CENTRE, "Fruit Party");
    al_draw_text(font_start_button, al_map_rgb(start.r, start.g, start.b), display_width / 2, (display_height + font_size + space_between - font_button) / 2, ALLEGRO_ALIGN_CENTRE, "start");
    al_draw_text(font_options, al_map_rgb(mode.r, mode.g, mode.b), margin, display_height - margin - font_button, 0, "change mode");
};

bool clickButton(const Button& button) {

    if (al_mouse_button_down(&mouse_state, 1)) {
        if (mouse_x >= button.x1 && mouse_x <= button.x2) {
            if (mouse_y >= button.y1 && mouse_y <= button.y2) return true;
            else return false;
        }
        else return false;
    }
    else return false;
};

void drawCharlie() {

    if (charlie_x < mouse_x) charlie_direction = charlie_right;//zmiana kierunku chodzenia
    else if (charlie_x > mouse_x) charlie_direction = charlie_left;

    charlie_x = mouse_x;

    if (charlie_x >= display_width - charlie_width) { //ogranicza zakres poruszania sie postaci do szerokosci ekranu
        charlie_x = display_width - charlie_width;
    }
    else if (charlie_x <= 0) {
        charlie_x = 0;
    }

    al_draw_bitmap_region(charlie_direction, (charlie_frame % 8) * charlie_width, 0, charlie_width, charlie_height, charlie_x, display_height - charlie_height, 0); //renderuje klatke animacji postaci;

    if ((al_get_timer_count(timer) % 4) == 0) { //co cztery uderzenia timera zmienia klatke animacji na kolejna - odpowiada za predkosc odtwarzania animacji
        charlie_frame++;
    }
}

void generateFruit() {

    if (rand_bonus <= 0) {
        new_fruit.fruit_type = bonus;
        new_fruit.sx = 0;
        new_fruit.sy = 0;
        new_fruit.dx = rand() % ((int)display_width - 100) + fruit_width;
        new_fruit.dy = -fruit_height;
        rand_bonus = rand() % 20 + 20;
    }
    else {
        if (rand_rotten <= 0) {
            new_fruit.fruit_type = rotten;
            rand_rotten = rand() % 8 + 1; //losuje, kiedy nastepnym razem pojawi sie zgnitek na ekranie
        }
        else new_fruit.fruit_type = bfruits;

        new_fruit.sx = (rand() % 12) * fruit_width; //losuje typ owoca - jego wspolrzedne na bitmapie
        new_fruit.sy = (rand() % 3) * fruit_height;
        new_fruit.dx = rand() % ((int)display_width - 100) + fruit_width; //losuje, w ktorym miejscu na ekranie bedzie spadal
        new_fruit.dy = -fruit_height; //zaczyna rysowac owoc poza gorna krawedzia ekranu
    }

    fruits.insert(fruits.begin(), new_fruit);
    fruit_generated = true;
}

void drawFruit() {

    for (int k = fruits.size() - 1; k >= 0; k--) {
        fruits[k].dy += speed;
        if (fruits[k].fruit_type != bonus) al_draw_bitmap_region(fruits[k].fruit_type, fruits[k].sx, fruits[k].sy, fruit_width, fruit_height, fruits[k].dx, fruits[k].dy, 0);
        else al_draw_bitmap(bonus, fruits[k].dx, fruits[k].dy, 0);

    }
}

void catchDrop() {

    for (int k = fruits.size() - 1; k >= 0; k--) {
        if ((fruits[k].dy + fruit_height > display_height - charlie_height - 15) && (fruits[k].dx > charlie_x - fruit_width) && (fruits[k].dx < charlie_x + charlie_width)) {//owoc zostaje zlapany

            if (fruits[k].fruit_type == bonus) current += 10;
            else if (fruits[k].fruit_type == rotten) hearts--;
            else current++;
            fruits.erase(fruits.begin() + k);
        }
    }

    if (!fruits.empty()) {
        if (fruits[fruits.size() - 1].dy + fruit_height > display_height) {//owoc spada na ziemie
            if (fruits[fruits.size() - 1].fruit_type == bfruits) hearts--;
            fruits.pop_back();
        }
    }

}

void changefruitTimer() {

    if (event.type == ALLEGRO_EVENT_TIMER) {
        if (event.timer.source == timer_fruit) {
            fruit_generated = false;
            al_set_timer_speed(timer_fruit, al_get_timer_speed(timer_fruit) * 0.99); //z kazdym wygenerowaniem owocu zwieksza czestotliwosc o 1% obecnej czestotliwosci
            if (al_get_timer_count(timer_fruit) % 30 == 0) speed++; //co 30 owocow zwieksza predkosc spadania
        }

    }
}

void drawHearts() {

    for (int k = 0; k < hearts; k++) {
        al_draw_bitmap(heart, k * 48 + 20, 20, 0);
    }
}

void drawEndScreen(const Color& button, const Color& background, const Color& text) {


    al_clear_to_color(al_map_rgb(background.r, background.g, background.b));
    al_draw_text(font_heading, al_map_rgb(text.r, text.g, text.b), display_width / 2, (display_height - space_between) / 2 - font_size - score_size - margin, ALLEGRO_ALIGN_CENTER, "Your score");
    al_draw_textf(font_start_button, al_map_rgb(text.r, text.g, text.b), display_width / 2, (display_height - space_between) / 2 - score_size, ALLEGRO_ALIGN_CENTER, "%d", current);
    al_draw_text(font_heading, al_map_rgb(text.r, text.g, text.b), display_width / 2, (display_height + space_between) / 2, ALLEGRO_ALIGN_CENTER, "Your best score");
    al_draw_textf(font_start_button, al_map_rgb(text.r, text.g, text.b), display_width / 2, (display_height + space_between) / 2 + font_size + margin, ALLEGRO_ALIGN_CENTER, "%d", best);
    al_draw_text(font_options, al_map_rgb(button.r, button.g, button.b), display_width - margin, margin, ALLEGRO_ALIGN_RIGHT, "back");

}

int main()
{
    al_init(); //inicjalizacja bibliotek Allegro
    if (!al_init()) { //wyswietlenie informacji o niepowodzeniu inicjalizacji
        al_show_native_message_box(NULL, "Error", "Allegro 5 initialisation error has occurred.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    if (!al_init_image_addon() || !al_init_font_addon() || !al_init_ttf_addon()) {
        al_show_native_message_box(NULL, "Error", "Addon initialisation error has occurred.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    display = al_create_display(960, 540);
    if (!display) {
        al_show_native_message_box(NULL, "Error", "Display creation error has occurred.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    timer = al_create_timer(1.0 / 60);
    timer_fruit = al_create_timer(2.0); //kontroluje czestotliwosc spadania owocow
    if (!timer || !timer_fruit) {
        al_show_native_message_box(NULL, "Error", "Timer creation error has occurred.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    //ladowanie bitmap i czcionek
    loadFiles();
    if (!charlie_left || !charlie_right || !bfruits || !rotten || !heart || !bonus) { //wyswietlenie informacji o bledzie ladowania bitmapy
        al_show_native_message_box(display, "Error", "Bitmap loading error has occurred.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }
    if (!font_heading || !font_start_button || !font_options) { //wyswietlenie informacji o bledzie ladowania czcionki
        al_show_native_message_box(display, "Error", "Font loading error has occurred.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    //instalacja klawiatury i myszki
    al_install_keyboard();
    al_install_mouse();

    //przekazanie informacji o eventach do kolejki
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_timer_event_source(timer_fruit));

    display_width = al_get_display_width(display), display_height = al_get_display_height(display);

    Button start, back, mode; //wspolrzedne przycisku start i back
    start.x1 = display_width / 2 - al_get_text_width(font_start_button, "start") / 2;
    start.x2 = display_width / 2 + al_get_text_width(font_start_button, "start") / 2;
    start.y1 = (display_height + font_size + space_between - font_button) / 2;
    start.y2 = (display_height + font_size + space_between + font_button) / 2;

    back.x1 = display_width - margin - al_get_text_width(font_options, "back");
    back.x2 = display_width - margin;
    back.y1 = margin;
    back.y2 = margin + font_button;

    mode.x1 = margin;
    mode.x2 = margin + al_get_text_width(font_options, "change mode");
    mode.y1 = display_height - margin - font_button;
    mode.y2 = display_height - margin;

    Color green = { 146, 146, 8 }, beige = { 255,194,132 }, plum1 = { 55,12,74 }, plum2 = { 204,153,204 }, orange = { 255,146,1 }, custom1 = { orange.r,orange.g,orange.b }, custom2 = { beige.r,beige.g,beige.b };


    al_start_timer(timer);

    while (running) {

        al_wait_for_event(queue, &event);
        if ((event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) || (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)) { //program konczy dzialanie po wcisnieciu Esc lub krzyzyka
            running = false;
        }

        if (event.type == ALLEGRO_EVENT_TIMER) {

            al_get_mouse_state(&mouse_state);
            mouse_x = al_get_mouse_state_axis(&mouse_state, 0);
            mouse_y = al_get_mouse_state_axis(&mouse_state, 1);

            //instrukcje zwiazane z przyciskiem start
            if (start_page) {
                if (start_executed && !al_mouse_button_down(&mouse_state, 1)) {//jesli klawisz byl przytrzymywany, a teraz zostal puszczony to gra sie rozpoczyna
                    start_page = false;
                    start_executed = false;
                    game = true;
                    timer_started = false;
                }
                else if (clickButton(start)) {//nacisnieto przycisk
                    drawStartScreen(custom2, custom1, custom2, custom2);  //animacja wcisniecia przycisku start
                    start_executed = true;
                }
                else if (!start_executed) drawStartScreen(green, custom1, custom2, custom2); //stan bierny

                //instrukcje zwiazane z przyciskiem change mode

                if (mode_executed && !al_mouse_button_down(&mouse_state, 1)) {

                    if (changed == false) {
                        if (custom1.r == orange.r) {//zmiana kolorow na przeciwny do obecny
                            custom1.r = plum1.r;
                            custom1.g = plum1.g;
                            custom1.b = plum1.b;
                            custom2.r = plum2.r;
                            custom2.g = plum2.g;
                            custom2.b = plum2.b;
                        }
                        else {
                            custom1.r = orange.r;
                            custom1.g = orange.g;
                            custom1.b = orange.b;
                            custom2.r = beige.r;
                            custom2.g = beige.g;
                            custom2.b = beige.b;
                        }
                        changed = true;
                    }
                }
                else if (clickButton(mode)) {
                    drawStartScreen(green, custom1, custom2, green); //animacja wcisniecia przycisku change mode
                    mode_executed = true;
                    changed = false;
                }
            }
            else if (game) { //instrukcje zwiazane z rozgrywka

                if (!timer_started) {
                    al_start_timer(timer_fruit);
                    timer_started = true;
                }
                if (!srand_init) {
                    srand(time(NULL));
                    srand_init = true;
                }

                al_clear_to_color(al_map_rgb(custom2.r, custom2.g, custom2.b));
                drawCharlie();
                al_draw_textf(font_start_button, al_map_rgb(custom1.r, custom1.g, custom1.b), display_width - margin, margin, ALLEGRO_ALIGN_RIGHT, "%d", current);

                if (!fruit_generated) generateFruit();
                drawFruit();
                catchDrop();
                changefruitTimer();
                if (!fruit_generated) {
                    rand_rotten--;
                    rand_bonus--;
                }

                if (hearts <= 0) {
                    game = false;
                    game_over = true;
                    end_page = true;
                }
                drawHearts();
            }
            else if (game_over) { //instrukcje zwiazane z koncem gry

                al_stop_timer(timer_fruit);

                if (!saved) {
                    int h = 0;
                    std::fstream file;
                    file.open("best.txt", std::ios::in);
                    if (file) {

                        while (1) {

                            if (file.eof()) break;
                            h = h * 10;
                            file >> best;
                            h += best;
                        }
                        best = h;
                        file.close();

                        if (current > best) {
                            file.open("best.txt", std::ios::out);
                            if (file) {
                                file << current;
                                file.close();
                                best = current;
                            }
                            else {
                                al_show_native_message_box(display, "Error", "Could not open a file for writing.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
                                running = false;
                            }
                        }
                    }
                    else {
                        al_show_native_message_box(display, "Error", "Could not open a file for reading.", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
                        running = false;
                    }
                    saved = true;
                }

                //instrukcje zwiazane z przyciskiem back
                if (back_executed && !al_mouse_button_down(&mouse_state, 1)) { //powrot do strony startowej

                    hearts = 3; //reset 
                    speed = 3;
                    fruits.clear();
                    fruit_generated = false;
                    current = 0;
                    rand_bonus = rand() % 20 + 20;
                    rand_rotten = rand() % 9;
                    al_set_timer_speed(timer_fruit, 2.0);

                    start_page = true;
                    back_executed = false;
                    game_over = false;
                    end_page = false;

                }
                else if (end_page && clickButton(back)) {
                    drawEndScreen(green, custom1, custom2);
                    back_executed = true;
                }
                else if (end_page && !back_executed) drawEndScreen(custom2, custom1, custom2);
            }
            al_flip_display();

        }
    }

    //wyczyszczenie pamieci, odinstalowanie klawiatury i myszki
    al_destroy_display(display);
    al_uninstall_keyboard();
    al_uninstall_mouse();
    al_destroy_timer(timer);
    al_destroy_timer(timer_fruit);
    al_destroy_font(font_heading);
    al_destroy_font(font_start_button);
    al_destroy_font(font_options);
    al_destroy_bitmap(charlie_right);
    al_destroy_bitmap(charlie_left);
    al_destroy_bitmap(charlie_direction);
    al_destroy_bitmap(bfruits);
    al_destroy_bitmap(rotten);
    al_destroy_bitmap(heart);
    al_destroy_bitmap(bonus);

    return 0;
}

