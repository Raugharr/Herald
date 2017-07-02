/*
 * File: GuiAux.c
 * Author: David Brotz
 */

#include "GuiAux.h"

#include "AABB.h"
#include "TextRenderer.h"

#include "../World.h"

#include "../sys/LuaCore.h"

#include <stdlib.h>
#include <assert.h>

#include <SDL2/SDL.h>

struct Label* CreateLabel(void) {
	struct Label* Label = (struct Label*) malloc(sizeof(struct Label));

	memset(Label, 0, sizeof(struct Label));
	return Label;
}

void LabelSetPosition(struct Widget* Widget, const struct SDL_Point* Pos) {
	struct Label* Label = (struct Label*) Widget;
	WidgetSetPosition(Widget, Pos);
	//Center text.
	Label->LabelPos.x = Widget->Rect.x + ((Widget->Rect.w - Label->LabelPos.w) / 2);
	Label->LabelPos.y = Widget->Rect.y + ((Widget->Rect.h - Label->LabelPos.h) / 2);
}

void ConstructLabel(struct Label* Widget, struct Container* Parent, const char* Text) {
	Widget->Widget.Rect.w = 0;
	Widget->Widget.Rect.h = 0;
	if(Widget->Widget.Style == NULL) Widget->Widget.Style = Parent->Skin->Label;
	ConstructWidget((struct Widget*)Widget, Parent, &Widget->Widget.Rect, LOBJ_LABEL);
	LabelSetText(&Widget->Widget, Text);
	Widget->Widget.OnDraw = LabelOnDraw;
	Widget->Widget.OnDestroy = DestroyLabel;
	Widget->SetText = LabelSetText;
	Widget->Widget.SetPosition = LabelSetPosition;
	SDL_SetTextureColorMod(((struct Label*)Widget)->Text, Widget->Widget.Style->FontUnfocus.r, Widget->Widget.Style->FontUnfocus.g, Widget->Widget.Style->FontUnfocus.b);
}

void DestroyLabel(struct Widget* Widget) {
	struct Label* Text = (struct Label*) Widget;
	SDL_DestroyTexture(Text->Text);
	DestroyWidget((struct Widget*)Text);
}

int LabelOnDraw(struct Widget* Widget) {
	struct Label* Label = (struct Label*) Widget;

	if(WidgetOnDraw(Widget) == 0) return 1;
	//FIXME: Shouldn't computing the center of the label be done only when the label has been changed?
	//Position the text in the middle of the label.
	return (SDL_RenderCopy(g_Renderer, Label->Text, NULL, &Label->LabelPos) == 0);
}

void LabelSetText(struct Widget* Widget, const char* Text) {
	struct Label* Label = (struct Label*) Widget;
	static const SDL_Color White = {255, 255, 255, 255};

	if(Label->Text != NULL)
		SDL_DestroyTexture(Label->Text);
	if(Text == NULL) {
		Label->Text = NULL;
		return;
	}
	if((Label->Text = CreateSolidText(Widget->Style->Font, Text, &White)) == NULL) {
		Log(ELOG_ERROR, "Cannot set label's text to %s.", Text);
		return;
	}
	SDL_QueryTexture(Label->Text, NULL, NULL, &Label->LabelPos.w, &Label->LabelPos.h);
	if(Widget->Rect.w == 0) Widget->Rect.w = Label->LabelPos.w * 1.25f;
	if(Widget->Rect.h == 0) Widget->Rect.h = Label->LabelPos.h * 1.25f;
	//Center text.
	Label->LabelPos.x = Widget->Rect.x + ((Widget->Rect.w - Label->LabelPos.w) / 2);
	Label->LabelPos.y = Widget->Rect.y + ((Widget->Rect.h - Label->LabelPos.h) / 2);
}

/*void LabelWordWrap(struct Label* Label, const char* Text) {
	struct Container* Parent = Label->Widget.Parent;
	struct Container* NewContainer = NULL;
	struct Font* Font = NULL;
	const char* Temp = Text;
	int CharWidth = 0;
	int WordSz = 0;
	int WordWidth = 0;
	SDL_Rect Rect = {0, 0, 0, 0};
	SDL_Rect PRect = {0, 0, 0, 0};
	SDL_Surface* Surface = NULL;
	SDL_Color White = {255, 255, 255};
	int Ct = 0;

	Font = Parent->Skin->Label->Font;
	Rect.w = TTF_FontFaceIsFixedWidth(Font->Font);
	Rect.x = 0;
	Rect.y = 0;
	PRect.w = Parent->Widget.Rect.w;
	PRect.h = Parent->Widget.Rect.h;
	NewContainer = CreateContainer();
	ConstructContainer(NewContainer, Parent, &PRect, State);
	NewContainer->NewChild = VertConNewChild;
	NewContainer->Widget.CanFocus = 0;
	while(*_Temp != '\0') {
		do {
			if((*_Temp) == ' ') {
				Ct += WordSz;
				Rect.w += WordWidth;
				WordSz = 0;
				WordWidth = 0;
			}
			TTF_GlyphMetrics(Font->Font, *_Temp, NULL, NULL, NULL, NULL, &CharWidth);
			WordWidth += CharWidth;
			if(Rect.w + WordWidth + CharWidth > Parent->Widget.Rect.w) {
				Temp -= WordSz;
				//NOTE: Used to stop infinite loop when the parent is to small to hold a single letter.
				if(WordSz == 0 && Temp == Text)
					goto func_end;
				WordSz = 0;
				WordWidth = 0;
				goto create_buffer;
				break;
			}
			++WordSz;
			++Temp;
		} while((*_Temp) != '\0');
		create_buffer:
		Ct += WordSz;
		Rect.w += WordWidth;
		WordSz = 0;
		WordWidth = 0;
		if(Ct > 0) {
			char Buffer[_Ct + 1];
			strncpy(Buffer, Text, Ct);
			Buffer[_Ct] = '\0';
			Label = CreateLabel();
			Surface = TTF_RenderText_Solid(Font->Font, Buffer, White);
			Rect.w = Surface->w;
			Rect.h = Surface->h;
			PRect.h += Rect.h;
			ConstructLabel(Label, NewContainer, &Rect, State, SurfaceToTexture(ConvertSurface(Surface)), Parent->Skin->Label);
		Text = Temp + 1;
		Label->Widget.CanFocus = 0;
		}
		Rect.w = 0;
		Ct = 0;
	}
	func_end:
	ContainerShrink(NewContainer);
	LuaCtor(State, NewContainer, LOBJ_CONTAINER);
	return 1;
}*/

struct Button* CreateButton(void) {
	struct Button* Button = (struct Button*) malloc(sizeof(struct Button));

	memset(Button, 0, sizeof(*Button));
	return Button;
}

struct Button* ConstructButton(struct Button* Widget, struct Container* Parent, const char* Text) {
	struct Label* Label = (struct Label*) Widget; 

	Widget->Label.Widget.Style = Parent->Skin->Button;
	ConstructLabel((struct Label*)Widget, Parent, Text);
	Label->Widget.OnDraw = ButtonOnDraw;
	Label->Widget.OnFocus = ButtonOnFocus;
	Label->Widget.OnUnfocus = ButtonOnUnFocus;
	Label->Widget.Style = Parent->Skin->Button;
	return Widget;
}

struct Widget* ButtonOnFocus(struct Widget* Widget, const SDL_Point* Point) {
	SDL_Color Out;

	if(PointInAABB(Point, &Widget->Rect) == SDL_FALSE || Widget->CanFocus == 0) {
		return NULL;
	}
	//GetBlendValue(&g_GuiStyles.FontUnfocus, &g_GuiStyles.FontFocus, &Out);
	Out = Widget->Style->FontFocus;
	SDL_SetTextureColorMod(((struct Label*)Widget)->Text, Out.r, Out.g, Out.b);
	return Widget;
}

int ButtonOnUnFocus(struct Widget* Widget) {
	const struct GuiStyle* Style = Widget->Style;

	SDL_SetTextureColorMod(((struct Label*)Widget)->Text, Style->FontUnfocus.r, Style->FontUnfocus.g, Style->FontUnfocus.b);
	return 1;
}

int ButtonOnDraw(struct Widget* Widget) {
	const struct GuiStyle* Style = Widget->Style;

	if(WidgetOnDraw(Widget) == 0) return 1;
	SDL_SetRenderDrawColor(g_Renderer, Style->Background.r, Style->Background.g, Style->Background.b, Style->Background.a);
	if(SDL_RenderFillRect(g_Renderer, &Widget->Rect) != 0) return 0;
	return LabelOnDraw(Widget); 
}

void ButtonSetClickable(struct Button* Button, int Clickable) {
	struct Label* Label = &Button->Label;

	if(Clickable == 1) {
		Label->Widget.OnDraw = ButtonOnDraw;
		Label->Widget.Clickable  = 1;
	} else {
		Label->Widget.OnDraw = LabelOnDraw;
		Label->Widget.Clickable  = 0;
	}
}

struct Table* CreateTable(void) {
	struct Table* Table = (struct Table*) malloc(sizeof(struct Table));

	memset(Table, 0, sizeof(*Table));
	return Table;
}

void ConstructTable(struct Table* Widget, struct Container* Parent, SDL_Rect* Rect,	int Columns, int Rows) {
	struct GuiStyle* Style = Parent->Skin->Table;
	int THeight = Style->Font->Height;
	int Size = Rows * Columns;

	Rect->h = THeight * Columns;
	WidgetSetLuaClass((struct Widget*)Widget, LOBJ_TABLE);
	ConstructContainer((struct Container*)Widget, Parent, Rect);
	Widget->Container.ChildrenSz = Columns * Rows;
	Widget->Container.Children = calloc(Widget->Container.ChildrenSz, sizeof(struct Widget*));
	Widget->Container.Widget.OnDestroy = DestroyTable;
	Widget->Container.NewChild = TableNewChild;
	Widget->Container.RemChild = DynamicRemChild;
	Widget->Columns = Columns;
	Widget->Rows = Rows;
	//_Widget->Container.VertFocChange = Rows;
	Widget->CellMax.w = 32;
	Widget->CellMax.h = 32;
	for(int i = 0; i < Size; ++i)
		Widget->Container.Children[i] = NULL;
}

void DestroyTable(struct Widget* Widget) {
	struct Table* Table = (struct Table*) Widget;
	DestroyContainer(&Table->Container.Widget);
}

void TableNewChild(struct Container* Parent, struct Widget* Child) {
	int Row = Parent->ChildCt - 1;
	int Col = 0;

	while(Row >= ((struct Table*)Parent)->Rows) {
		Row -= ((struct Table*)Parent)->Rows;
		++Col;
	}
	Child->Rect.x = Parent->Widget.Rect.x + (Row * ((struct Table*)Parent)->CellMax.w);
	Child->Rect.y = Parent->Widget.Rect.y + (Col * ((struct Table*)Parent)->CellMax.h);
	if(Child->Rect.w > ((struct Table*)Parent)->CellMax.w)
		Child->Rect.w = ((struct Table*)Parent)->CellMax.w;
	if(Child->Rect.h > ((struct Table*)Parent)->CellMax.h)
		Child->Rect.h = ((struct Table*)Parent)->CellMax.h;
}

struct TextBox* CreateTextBox(void) {
	struct TextBox* TextBox = (struct TextBox*) malloc(sizeof(struct TextBox));

	memset(TextBox, 0, sizeof(*TextBox));
	return TextBox;
}

void DestroyTextBox(struct Widget* Widget) {
	struct TextBox* TextBox = (struct TextBox*) Widget;
	SDL_StopTextInput();
	DtorTextStream(&TextBox->Stream);
	DestroyWidget(&TextBox->Widget);
}

void ConstructTextBox(struct TextBox* TextBox, struct Container* Parent, int Chars) {
	struct GuiStyle* Style = Parent->Skin->Default;

	SDL_Rect Rect = {0,	0, Chars * 8, 16};
	TextBox->Widget.Style = Parent->Skin->Default;
	ConstructWidget((struct Widget*)TextBox, Parent, &Rect, LOBJ_TEXTBOX); 
	CtorTextStream(&TextBox->Stream, Style->Font);
	TextBox->Widget.OnDraw = TextBoxOnDraw;
	TextBox->Widget.OnKey = TextBoxOnKey;
	TextBox->Widget.OnDestroy = DestroyTextBox;
	TextBox->Widget.OnFocus = TextBoxOnFocus;
	TextBox->Widget.OnUnfocus = TextBoxOnUnfocus;
}

void TextBoxOnKey(struct Widget* Widget, const union UWidgetOnKey* Event) {
	struct TextBox* TextBox = (struct TextBox*) Widget;
	char* Clipboard = NULL;

	if(Event->Type == WEVENT_KEY) {
		switch(Event->Key.Key) {
			case SDLK_BACKSPACE:
				TextStreamPop(&TextBox->Stream);
				break;
			case SDLK_RETURN:
				TextStreamClear(&TextBox->Stream);
				break;
			case SDLK_c:
				if((Event->Key.Mod & KMOD_CTRL) == 0) break;
				Clipboard = SDL_GetClipboardText();
				if(Clipboard == NULL) {
					Log(ELOG_ERROR, SDL_GetError());
					break;
				}
				for(int i = 0; Clipboard[i] != '\0'; ++i) {
					TextStreamPush(&TextBox->Stream, Clipboard[i]);
				}
				SDL_free(Clipboard);
				break;
		}
	} else if(Event->Type == WEVENT_TEXT) {
		TextStreamPush(&TextBox->Stream, Event->Text.Text[0]);
	}
}

int TextBoxOnDraw(struct Widget* Widget) {
	struct TextBox* TextBox = (struct TextBox*) Widget;

	if(WidgetOnDraw(Widget) == 0) return 1;
	if(SDL_RenderFillRect(g_Renderer, &Widget->Rect) != 0) return 0;
	SDL_RenderDrawRect(g_Renderer, &Widget->Rect);
	return (TextStreamRender(&TextBox->Stream, g_Renderer, &TextBox->Widget.Rect) == 0);
}

struct Widget* TextBoxOnFocus(struct Widget* Widget, const SDL_Point* Point) {
	if(WidgetOnFocus(Widget, Point) == NULL) return NULL;
	SDL_StartTextInput();
	return Widget;
}

int TextBoxOnUnfocus(struct Widget* Widget) {
	SDL_StopTextInput();
	return 1;
}

struct ContextItem* CreateContextItem(void) {
	return (struct ContextItem*) malloc(sizeof(struct ContextItem));
}

void ConstructContextItem(struct ContextItem* Widget, struct Container* Parent, SDL_Rect* Rect) {
	ConstructContainer((struct Container*)Widget, Parent, Rect);
	Widget->Container.Widget.OnDraw = (int(*)(struct Widget*))ContextItemOnDraw;
	Widget->Container.Widget.OnFocus = (struct Widget* (*)(struct Widget*, const SDL_Point*))ContextItemOnFocus;
	Widget->Container.Widget.OnUnfocus = (int(*)(struct Widget*))ContextItemOnUnfocus;
	Widget->Container.NewChild = ContextItemNewChild;
	//_Widget->Container.VertFocChange = 0;
	Widget->ShowContexts = 0;
}

int ContextItemOnDraw(struct ContextItem* Container) {
	if(Container->ShowContexts != 0) {
		for(int i = 1; i < Container->Container.ChildCt; ++i)
			Container->Container.Children[i]->OnDraw(Container->Container.Children[i]);
	}
	if(Container->Container.ChildCt > 0)
		Container->Container.Children[0]->OnDraw(Container->Container.Children[0]);
	return 1;
}

struct Widget* ContextItemOnFocus(struct ContextItem* Widget, const SDL_Point* Point) {
	ContainerOnFocus(&Widget->Container.Widget, Point);
	Widget->ShowContexts = 1;
//	Widget->Container.VertFocChange = Widget->Container.ChildCt;
	return (struct Widget*) Widget;
}

int ContextItemOnUnfocus(struct ContextItem* Widget) {
	ContainerOnUnfocus(&Widget->Container.Widget);
	Widget->ShowContexts = 0;
	return 1;
}

void ConstructConsole(struct Console* Console, struct Container* Parent, int Rows, int Cols) {
	uint16_t h = 0;
	const struct GuiStyle* Style = Parent->Skin->Label;
	SDL_Rect Rect = {0};

	Assert(Rows > 0);
	Assert(Cols > 1);

	h = Style->Font->Height;
	//NOTE: This is to create a rough estimate of how big a text box that can contain Cols characters. Obviously multiplying Cols by 8 will not work for all font sizes and should be improved.
	Rect.w = Cols * 8;
	Rect.h = Rows * h;
	ConstructContainer(&Console->Container, Parent, &Rect);
	Console->Container.NewChild = VertConNewChild; 
	Console->Container.Widget.OnKey = ConsoleOnKey;
	memset(Console->History, 0, sizeof(struct Label) * Rows);
	Console->HistorySize = Rows;
	CtorArray(&Console->TextArr, Rows);
	for(int i = 0; i < Rows; ++i) {
		ConstructLabel(&Console->History[i], &Console->Container, NULL);
	}
	ConstructTextBox(&Console->TextBox, &Console->Container, Rows);
}

void ConsoleOnKey(struct Widget* Widget, const union UWidgetOnKey* Event) {
	struct Console* Console = (struct Console*) Widget;
	struct TextBox* TextBox = &Console->TextBox;

	if(Event->Type == WEVENT_TEXT) {
		switch(Event->Key.Key) {
			case SDLK_RETURN:
				if(Console->Top == Console->HistorySize)
					SDL_DestroyTexture(Console->History[Console->Top - 1].Text);
				for(int i = Console->Top - 1; i > 0; --i) {
					Console->History[i].Text = Console->History[i - 1].Text;
				}
				LabelSetText(&Console->History[0].Widget, TextBox->Stream.Stream);
				break;
		}
	}
}
