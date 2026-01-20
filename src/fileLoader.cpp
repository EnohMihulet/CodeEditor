#include <fstream>
#include <string>

#include "fileLoader.h"
#include "commonTypes.h"

static std::string extractFileName(const std::string& path) {
	size_t sep = path.find_last_of("/\\");
	if (sep == std::string::npos || sep + 1 >= path.size()) return path;
	return path.substr(sep + 1);
}

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
	st.currentFilePath = path;
	st.currentFileName = extractFileName(path);
	st.isDirty = false;

	return OK;
}
