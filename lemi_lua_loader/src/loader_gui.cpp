#include "loader_gui.h"

#include <future>


#include "imgui/imgui.h"
#include "imgui/TextEditor.h"
#include "lua_loader.h"

#include <mutex>
#include <thread>

TextEditor* editor;

void loader_gui::draw()
{
	static std::once_flag f;
	std::call_once(f, []()
	{
		editor = new TextEditor();
		editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
		editor->SetShowWhitespaces(false);
	});

	if (!is_gui_open)
	{
		ImGui::GetIO().MouseDrawCursor = false;
		return;
	}

	ImGui::GetIO().MouseDrawCursor = true;
	
	auto cpos = editor->GetCursorPosition();
	ImGui::Begin("LemiLoader", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Edit"))
		{
			bool ro = editor->IsReadOnly();
			if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
				editor->SetReadOnly(ro);
			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor->CanUndo()))
				editor->Undo();
			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor->CanRedo()))
				editor->Redo();

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor->HasSelection()))
				editor->Copy();
			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor->HasSelection()))
				editor->Cut();
			if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor->HasSelection()))
				editor->Delete();
			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
				editor->Paste();

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", nullptr, nullptr))
				editor->SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor->GetTotalLines(), 0));

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Dark palette"))
				editor->SetPalette(TextEditor::GetDarkPalette());
			if (ImGui::MenuItem("Light palette"))
				editor->SetPalette(TextEditor::GetLightPalette());
			if (ImGui::MenuItem("Retro blue palette"))
				editor->SetPalette(TextEditor::GetRetroBluePalette());
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor->GetTotalLines(),
		editor->IsOverwrite() ? "Ovr" : "Ins",
		editor->CanUndo() ? "*" : " ",
		editor->GetLanguageDefinition().mName.c_str());

	editor->Render("TextEditor", {
		               ImGui::GetContentRegionAvail().x,
		               ImGui::GetContentRegionAvail().y - (ImGui::CalcTextSize("Run").y + ImGui::CalcTextSize("Run").y / 2)
	               });

	if (ImGui::Button("Run##RUN_SCRIPT")) auto as = std::async(std::launch::async, []()
	{
		lua_loader::load_lua(editor->GetText());
	});
	
	ImGui::End();
}