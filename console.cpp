
#include "ici.h"

#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <deque>

#ifdef WIN32
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static HANDLE ECCON; // The console
#else
#include "unistd.h"
#endif

struct ICICommand {
	void *context;
	virtual void operator()(const std::string &command, std::string &line) {}
};

struct ICIHelpCommand : public ICICommand {
	virtual void operator()(const std::string &command, std::string &line);
} iciHelpCommand;
struct ICIHistoryCommand : public ICICommand {
	virtual void operator()(const std::string &command, std::string &line);
} iciHistoryCommand;
struct ICIClearCommand : public ICICommand {
	virtual void operator()(const std::string &command, std::string &line);
} iciClearCommand;
struct ICIListCommand : public ICICommand {
	virtual void operator()(const std::string &command, std::string &line);
} iciListCommand;

class ICIConsole {
public:
	char InputBuf[256];
	bool ScrollToBottom;
	int HistoryPos; // -1: new line, 0..History.Size-1 browsing history.
	ImVector<char*> Items;
	std::deque<std::string> History;
	std::unordered_map<std::string, ICICommand*> Commands;

	ICIConsole() {
		ClearLog();
		memset(InputBuf, 0, sizeof(InputBuf));
		HistoryPos = -1;
		Commands["help"] = &iciHelpCommand;
		Commands["history"] = &iciHistoryCommand;
		Commands["clear"] = &iciClearCommand;
		Commands["list"] = &iciListCommand;
		AddLogText("Console Loaded");
	}
	~ICIConsole() {
		ClearLog();
	}

	// Portable helpers
	static int   Strnicmp(const char* str1, const char* str2, int n) { int d = 0; while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; n--; } return d; }
	static char* Strdup(const char *str)							 { size_t len = strlen(str) + 1; void* buff = malloc(len); return (char*)memcpy(buff, (const void*)str, len); }

	void ClearLog() {
		for (int i = 0; i < Items.Size; i++)
			free(Items[i]);
		Items.clear();
		ScrollToBottom = true;
	}

	void AddLogText(const char* txt) {
		Items.push_back(Strdup(txt));
		ScrollToBottom = true;
	}

	void AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		buf[sizeof(buf) - 1] = 0;
		va_end(args);
		Items.push_back(Strdup(buf));
		ScrollToBottom = true;
	}

	void Draw(const char* title, bool* p_open) {
		ImGui::SetNextWindowSize(ImVec2(520,600), ImGuiSetCond_FirstUseEver);
		if(!ImGui::Begin(title, p_open, ImGuiWindowFlags_ShowBorders)) {
			ImGui::End();
			return;
		}

		if (ImGui::SmallButton("Clear")) ClearLog(); ImGui::SameLine();
		if (ImGui::SmallButton("Scroll to bottom")) ScrollToBottom = true;

		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
		static ImGuiTextFilter filter;
		filter.Draw("Filter (\"incl,-excl\")", 180);
		ImGui::PopStyleVar();
		ImGui::Separator();

		ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
		for (int i = 0; i < Items.Size; i++) {
			const char* item = Items[i];
			if (!filter.PassFilter(item))
				continue;
			ImVec4 col = ImVec4(1.0f,1.0f,1.0f,1.0f);
			if (strstr(item, "[error]")) col = ImColor(1.0f,0.4f,0.4f,1.0f);
			else if (strncmp(item, "# ", 2) == 0) col = ImColor(1.0f,0.78f,0.58f,1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImColor(0.2f,1.0f,0.55f,1.0f));
			ImGui::TextUnformatted(">>");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted(item);
			ImGui::PopStyleColor();
		}
		if (ScrollToBottom)
			ImGui::SetScrollHere();
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		if (ImGui::InputText("Input", InputBuf, sizeof(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this)) {
			char* input_end = InputBuf+strlen(InputBuf);
			while (input_end > InputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
			if (InputBuf[0])
				ExecCommand(InputBuf);
			InputBuf[0] = 0;
		}

		// Demonstrate keeping auto focus on the input box
		if((ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
			ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

		ImGui::End();
	}

	void ExecCommand(const char* command_line_cstr) {
		std::string command = command_line_cstr;
		std::string command_line = command_line_cstr;
		AddLog("# %s\n", command_line_cstr);

		// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
		HistoryPos = -1;
		for (auto itr = History.begin(); itr != History.end(); itr++) {
			if (*itr == command) {
				History.erase(itr);
				break;
			}
		}
		History.push_back(command_line);

		auto com_pair = Commands.find(command);
		if (com_pair == Commands.cend()) {
			AddLog("Unknown command: '%s'", command_line_cstr);
		} else {
			// Process command
			(*com_pair->second)(command, command);
		}

		while(History.size() > 100) {
			History.pop_front();
		}
	}

	static int TextEditCallbackStub(ImGuiTextEditCallbackData* data) {
		ICIConsole* console = (ICIConsole*)data->UserData;
		return console->TextEditCallback(data);
	}

	int TextEditCallback(ImGuiTextEditCallbackData* data) {
		switch (data->EventFlag) {
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf) {
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			// Build a list of candidates
			ImVector<const char*> candidates;
			for (auto &cm : Commands)
				if (Strnicmp(cm.first.c_str(), word_start, (int)(word_end-word_start)) == 0)
					candidates.push_back(cm.first.c_str());

			if (candidates.Size == 0) {
				// No match
				AddLog("No match for \"%.*s\"!\n", (int)(word_end-word_start), word_start);
			} else if (candidates.Size == 1) {
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
				data->DeleteChars((int)(word_start-data->Buf), (int)(word_end-word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			} else {
				// Multiple matches. Complete as much as we can
				int match_len = (int)(word_end - word_start);
				for (;;) {
					int c = 0;
					bool all_candidates_matches = true;
					for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
						if (i == 0)
							c = tolower(candidates[i][match_len]);
						else if (c != tolower(candidates[i][match_len]))
							all_candidates_matches = false;
					if (!all_candidates_matches)
						break;
					match_len++;
				}

				if (match_len > 0)
				{
					data->DeleteChars((int)(word_start - data->Buf), (int)(word_end-word_start));
					data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
				}
				// List matches
				AddLog("Possible matches:\n");
				for (int i = 0; i < candidates.Size; i++)
					AddLog("- %s\n", candidates[i]);
			}
		}
			break;
		case ImGuiInputTextFlags_CallbackHistory:
		{
			// Example of HISTORY
			const int prev_history_pos = HistoryPos;
			if (data->EventKey == ImGuiKey_UpArrow) {
				if (HistoryPos == -1)
					HistoryPos = ((int)History.size()) - 1;
				else if (HistoryPos > 0)
					HistoryPos--;
			} else if (data->EventKey == ImGuiKey_DownArrow) {
				if (HistoryPos != -1)
					if (++HistoryPos >= ((int)History.size()))
						HistoryPos = -1;
			}

			// A better implementation would preserve the data on the current input line along with cursor position.
			if (prev_history_pos != HistoryPos) {
				data->CursorPos =
				data->SelectionStart =
				data->SelectionEnd =
				data->BufTextLen =
					snprintf(data->Buf, (size_t)data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos].c_str() : "");
				data->BufDirty = true;
			}
		}
			break;
		default:
			break;
		}
		return 0;
	}
};
#if defined(WIN32) && defined(_MSC_VER) && (_MSC_VER < 1900)
int snprintf(char * buf, size_t len, const char * fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	return vsnprintf(buf, len, fmt, va);
}
#endif

static ICIConsole *uiconsole;
static bool uiconsole_late = false;
bool uiconsole_open = false;

void ICIHelpCommand::operator()(const std::string &command, std::string &line) {
	uiconsole->AddLog("Commands:");
	for (auto &cm : uiconsole->Commands) {
		uiconsole->AddLog("- %s", cm.first.c_str());
	}
}
void ICIHistoryCommand::operator()(const std::string &command, std::string &line) {
	int i = 0;
	int thresh = uiconsole->History.size();
	if (thresh > 10) thresh -= 10;
	else thresh = 0;
	for (auto &itr : uiconsole->History) {
		if (i >= thresh)
			uiconsole->AddLog("%3d: %s\n", i + 1, itr.c_str());
		i++;
	}
}
void ICIClearCommand::operator()(const std::string &command, std::string &line) {
	uiconsole->ClearLog();
}
void ICIListCommand::operator()(const std::string &command, std::string &line) {
	//
}


void StartConsole() {
#ifdef WIN32
	COORD dws = {80,400};
	AllocConsole();
	ECCON = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleScreenBufferSize(ECCON, dws);
	SetConsoleActiveScreenBuffer(ECCON);
#endif
	uiconsole = new ICIConsole;
}

void StartGUIConsole() {
	uiconsole_late = true;
}

void ShowUIConsole() {
	if(uiconsole_open)
		ImGui::SetWindowFocus("ICI Console");
	else
		uiconsole_open = true;
}

void DrawUIConsole() {
	if(uiconsole_open) uiconsole->Draw("ICI Console", &uiconsole_open);
}

IM_PRINTFARGS(1)
void LogMessage(const char *fmt, ...) {
	char conbuf[1024];
	int conlen;
	va_list val;
	va_start(val, fmt);
	conlen = vsnprintf(conbuf, sizeof(conbuf), fmt, val);
	conbuf[sizeof(conbuf)-1] = 0;
	va_end(val);
	if(!uiconsole_late) {
#ifdef WIN32
		DWORD cw;
		WriteConsoleA(ECCON, conbuf, conlen, &cw, NULL);
#else
		write(2, conbuf, conlen);
#endif
	}
	if(uiconsole) uiconsole->AddLogText(conbuf);
}

