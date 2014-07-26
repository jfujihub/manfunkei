#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SIZE_STR 256

#define SIZE_WINDOW_X 320
#define SIZE_WINDOW_Y 240


typedef struct {
	GtkWidget *window;
	GtkWidget *entry;
	GtkWidget *drawing_area;
	GdkPixmap *pixmap;
	double min; /* 計時する時間[分] */
	time_t start_time; /* タイマー開始時のエポック秒 */
	time_t last_time; /* 秒針を書き換えた最後のエポック秒 */
} COMMON_DATA; /* イベントコールバックで共通に使われるデータ */


void destroy(GtkWidget *widget, gpointer data)
/* 終了処理 */
{
	gtk_main_quit();
}


gint configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
/* ウインドウがリサイズされたときの処理 */
{
	COMMON_DATA *common_data = (COMMON_DATA *) data;

	/* pixmap を解放する */
	if (common_data->pixmap) {
		gdk_pixmap_unref(common_data->pixmap);
	}

	/* ウインドウのサイズに合わせて新たな pixmap を確保する */
	common_data->pixmap = gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);

	return TRUE;
}


gint expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
/* ウインドウが前面に出たときの処理 */
{
	COMMON_DATA *common_data = (COMMON_DATA *) data;

	/* pixmap をウインドウにコピー */
	gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], common_data->pixmap,
		event->area.x, event->area.y, event->area.x, event->area.y,
		event->area.width, event->area.height);

	return FALSE;
}


void input_entry_widget(GtkWidget *widget, gpointer data)
/* エントリーウィジェットの内容をカウントダウンの時間 (分) として読み込む */
{
	char entry_text[SIZE_STR];
	COMMON_DATA *common_data = (COMMON_DATA *) data;

	strcpy(entry_text, gtk_entry_get_text(GTK_ENTRY(common_data->entry)));
	common_data->min = atof(entry_text);

	if (common_data->min < 1.0) {
		gtk_entry_set_text(GTK_ENTRY(common_data->entry), "");
		return;
	}

	gtk_main_quit();
}


void draw_tic(cairo_t *cr, int hour, double size_circle, int fill)
/* 表示盤の目盛りを描画する */
{
	double tic_radian, tic_x, tic_y;

	tic_radian = 0.5 * M_PI - 2.0 * M_PI / 12.0 * hour;
	tic_x = SIZE_WINDOW_X * 0.5 + cos(tic_radian) * SIZE_WINDOW_X * 0.4;
	tic_y = SIZE_WINDOW_Y * 0.5 - sin(tic_radian) * SIZE_WINDOW_Y * 0.4;
	cairo_move_to(cr, tic_x + SIZE_WINDOW_X * size_circle, tic_y);
	cairo_arc(cr, tic_x, tic_y, SIZE_WINDOW_X * size_circle, 0.0, M_PI * 2.0);
	if (fill) {
		cairo_fill_preserve(cr);
	}
}


gint main_timer_event(gpointer data)
/* タイマーイベント発生時の処理を行う */
{
	char str[SIZE_STR];
	double hand_radian, hand_x, hand_y;
	time_t epoch_time;
	COMMON_DATA *common_data = (COMMON_DATA *) data;
	cairo_t *cr;
	GdkRectangle update_rect; /* expose イベントによって更新される範囲 */

	/* 前回の画面更新より 1 秒経っていないときは何もせずに関数を抜ける */
	if (common_data->last_time == time(NULL)) {
		return TRUE;
	}
	common_data->last_time = time(NULL);

	/* cairo を使って pixmap に描画する準備 */
	cr = gdk_cairo_create(common_data->pixmap);

	/* 画面消去 */
	cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
	cairo_paint(cr);

	/* 目盛り (tic) */
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 2);
	draw_tic(cr, 0, 0.03, TRUE);
	draw_tic(cr, 1, 0.01, FALSE);
	draw_tic(cr, 2, 0.01, FALSE);
	draw_tic(cr, 3, 0.03, FALSE);
	draw_tic(cr, 4, 0.01, FALSE);
	draw_tic(cr, 5, 0.01, FALSE);
	draw_tic(cr, 6, 0.03, FALSE);
	draw_tic(cr, 7, 0.01, FALSE);
	draw_tic(cr, 8, 0.01, FALSE);
	draw_tic(cr, 9, 0.03, FALSE);
	draw_tic(cr, 10, 0.01, FALSE);
	draw_tic(cr, 11, 0.01, FALSE);
	cairo_stroke(cr);

	/* 針 (hand) */
	time(&epoch_time);
	if (common_data->start_time + common_data->min * 60 < epoch_time) {
		epoch_time = common_data->start_time + common_data->min * 60;
	}
	hand_radian = 0.5 * M_PI - (epoch_time - common_data->start_time) / (common_data->min * 60.0) * 2.0 * M_PI;
	hand_x = SIZE_WINDOW_X * 0.5 + cos(hand_radian) * SIZE_WINDOW_X * 0.32;
	hand_y = SIZE_WINDOW_Y * 0.5 - sin(hand_radian) * SIZE_WINDOW_Y * 0.32;
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	cairo_set_line_width(cr, 3);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(cr, SIZE_WINDOW_X * 0.5, SIZE_WINDOW_Y * 0.5);
	cairo_line_to(cr, hand_x, hand_y);
	cairo_stroke(cr);

	/* pixmap への描画終了 */
	cairo_destroy(cr);

	/* 更新する画面範囲を矩形で設定する */
	update_rect.x = 0;
	update_rect.y = 0;
	update_rect.width = common_data->drawing_area->allocation.width;
	update_rect.height = common_data->drawing_area->allocation.height;
	/* drawing_area を描き換えるための expose event シグナルを発行する */
	gtk_widget_draw(common_data->drawing_area, &update_rect);

	/* タイトルを更新する */
	sprintf(str, "万分計 - 残り秒数: %d", (int) (common_data->start_time + common_data->min * 60 - epoch_time));
	gtk_window_set_title(GTK_WINDOW(common_data->window), str);

	return TRUE;
}


int main(int argc, char **argv)
/* タイマーの練習 */
{
	GtkWidget *window; /* トップレベルのウィジェット */
	GtkWidget *label; /* テキスト表示用 (ラベル) ウィジェット */
	GtkWidget *entry; /* テキスト入力用 (エントリー) ウィジェット */
	GtkWidget *button; /* ボタンウィジェット */
	GtkWidget *pbox; /* パッキングボックス */
	GtkWidget *drawing_area; /* 描画領域 */
	gint main_timer; /* タイマーの ID */
	COMMON_DATA common_data; /* コールバック関数に渡すデータ */

	/* GTK+ の初期化およびコマンドライン引数解析 */
	gtk_init(&argc, &argv);

	/* ウィンドウの作成 */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	common_data.window = window;
	/* ウィンドウの大きさの設定 */
	gtk_widget_set_size_request(window, SIZE_WINDOW_X, SIZE_WINDOW_Y);
	/* メインウインドウのタイトルを設定する */
	gtk_window_set_title(GTK_WINDOW(window), "万分計");

	/* パッキングボックスの作成 */
	pbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(window), pbox);
	gtk_widget_show(pbox);

	/* ラベルウィジェットの作成 */
	label = gtk_label_new("何分間のカウントダウンですか？\n（１以上、小数入力可）");
	gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* エントリーウィジェットの作成 */
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(pbox), entry, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(entry), "activate", GTK_SIGNAL_FUNC(input_entry_widget), &common_data); /* activate: Enter キーが押下されたときに発生するシグナル */
	gtk_widget_show(entry);
	common_data.entry = entry;

	/* ボタンウィジェットの作成 */
	button = gtk_button_new_with_label("OK");
	gtk_box_pack_start(GTK_BOX(pbox), button, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(input_entry_widget), &common_data);
	gtk_widget_show(button);

	/* メインループから抜けるコールバック関数を登録 */
	gtk_signal_connect(GTK_OBJECT(window), "delete-event", GTK_SIGNAL_FUNC(destroy), NULL);

	/* ウィンドウの表示 */
	gtk_widget_show(window);

	/* メッセージループ (テキストエントリー入力待ち) */
	gtk_main();

	/* 入力に使ったウィジェットを非表示にする */
	gtk_widget_hide(label);
	gtk_widget_hide(entry);
	gtk_widget_hide(button);

	/* 描画領域の作成 */
	common_data.pixmap = NULL;
	drawing_area = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), SIZE_WINDOW_X, SIZE_WINDOW_Y);
	gtk_box_pack_start(GTK_BOX(pbox), drawing_area, FALSE, FALSE, 0);
	gtk_widget_show(drawing_area);
	common_data.drawing_area = drawing_area;
	g_signal_connect(G_OBJECT(drawing_area), "configure_event", (GtkSignalFunc) configure_event, &common_data);
	g_signal_connect(G_OBJECT(drawing_area), "expose_event", (GtkSignalFunc) expose_event, &common_data);

	/* タイマー設定・開始 */
	main_timer = gtk_timeout_add(333, (GtkFunction) main_timer_event, &common_data);
	common_data.start_time = time(NULL);
	common_data.last_time = time(NULL) - 1;

	/* メインループ */
	gtk_main();

	/* タイマーの破棄 */
	gtk_timeout_remove(main_timer);

	return EXIT_SUCCESS;
}

