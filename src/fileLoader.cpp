#include <fstream>

#include "fileLoader.h"
#include "commonTypes.h"

s32 loadFile(EditorState& st, const std::string& path) {

	std::ifstream file(path);

	if (!file) return ERR_FILE_NOT_FOUND;

	std::string line;
	TextBuffer* newText = new TextBuffer{};
	while (std::getline(file, line)) {
		newText->append(line, (u32)line.size());
	}

	delete st.Text;
	st.Text = newText;
	

	return OK;
}
