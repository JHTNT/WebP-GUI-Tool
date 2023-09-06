import sys

from PySide6.QtGui import QAction, QIcon
from PySide6.QtWidgets import (
    QApplication,
    QHBoxLayout,
    QListWidget,
    QMainWindow,
    QMenu,
    QMenuBar,
    QPushButton,
    QVBoxLayout,
    QWidget,
)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.main_widget = QWidget()

        self.settings_button = QPushButton("參數設定", self)
        self.start_button = QPushButton("開始", self)

        self.queueing_files_list = QListWidget(self)

        self.add_file_action = QAction("添加檔案", self)
        self.add_folder_action = QAction("添加資料夾", self)

        self.file_menu = QMenu("檔案", self)
        self.about_menu = QMenu("關於", self)

        self.menubar = QMenuBar(self)

        # layout of main window
        self.main_layout = QVBoxLayout()
        # layout of the buttons at the bottom
        self.function_bar_layout = QHBoxLayout()
        self.init_widget()
        self.init_window()

    def init_widget(self):
        self.setCentralWidget(self.main_widget)
        self.init_layout()

    def init_layout(self):
        self.main_layout.addWidget(self.queueing_files_list)

        self.function_bar_layout.addWidget(self.settings_button)
        self.function_bar_layout.addWidget(self.start_button)

        self.main_layout.addLayout(self.function_bar_layout)

        self.main_widget.setLayout(self.main_layout)

    def init_window(self):
        self.file_menu.addAction(self.add_file_action)
        self.file_menu.addAction(self.add_folder_action)
        self.menubar.addAction(self.file_menu.menuAction())
        self.menubar.addAction(self.about_menu.menuAction())
        self.setMenuBar(self.menubar)
        self.setWindowTitle("WebP GUI tool")
        self.setWindowIcon(QIcon(".\\app\\resources\\logo.ico"))
        # self.resize(991, 656)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
