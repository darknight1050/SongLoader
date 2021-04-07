#pragma once

namespace RuntimeSongLoader::LoadingUI {

    void CreateCanvas();

    void UpdateLoadingProgress(int maxFolders, int currentFolder);

    void UpdateLoadedProgress(int levelsCount, int time);

    void SetActive(bool active);
    
    void UpdateState();

}