import os
import sys

from components.dialogs.settings_dialog import SettingsDialog
from components.widgets.file_item import FileItem
from PySide6.QtCore import QSettings
from PySide6.QtGui import QAction, QIcon
from PySide6.QtWidgets import (
    QApplication,
    QFileDialog,
    QHBoxLayout,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMenu,
    QMenuBar,
    QPushButton,
    QStyle,
    QVBoxLayout,
    QWidget,
)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.main_widget = QWidget()
        self.settings_dialog = SettingsDialog(self)

        self.settings_button = QPushButton("參數設定", self)
        self.start_button = QPushButton("開始", self)

        self.queueing_files_list = QListWidget(self)

        self.add_file_action = QAction("添加檔案", self)
        self.add_directory_action = QAction("添加資料夾", self)

        self.file_menu = QMenu("檔案", self)
        self.about_menu = QMenu("關於", self)

        self.menubar = QMenuBar(self)

        # layout of main window
        self.main_layout = QVBoxLayout()
        # layout of the buttons at the bottom
        self.function_bar_layout = QHBoxLayout()

        self.cwebp_settings = QSettings("config.ini", QSettings.IniFormat)

        if os.path.exists("./config.ini"):
            self.apply_settings()
        else:
            self.save_settings()

        self.init_widget()
        self.init_layout()
        self.init_window()

    def init_widget(self):
        self.setCentralWidget(self.main_widget)

        self.settings_button.clicked.connect(self.open_setting)

        self.add_file_action.triggered.connect(self.add_file)
        self.add_directory_action.triggered.connect(self.add_directory)

    def init_layout(self):
        self.main_layout.addWidget(self.queueing_files_list)

        self.function_bar_layout.addWidget(self.settings_button)
        self.function_bar_layout.addWidget(self.start_button)

        self.main_layout.addLayout(self.function_bar_layout)

        self.main_widget.setLayout(self.main_layout)

    def init_window(self):
        self.file_menu.addAction(self.add_file_action)
        self.file_menu.addAction(self.add_directory_action)
        self.menubar.addAction(self.file_menu.menuAction())
        self.menubar.addAction(self.about_menu.menuAction())
        self.setMenuBar(self.menubar)
        self.setWindowTitle("WebP GUI tool")
        self.setWindowIcon(QIcon(".\\app\\resources\\logo.ico"))

    def apply_settings(self):
        self.settings_dialog.quality_spin.setValue(float(self.cwebp_settings.value("quality")))
        if self.cwebp_settings.value("compress_option") == "lossy":
            self.settings_dialog.lossy_radio.setChecked(True)
        if self.cwebp_settings.value("output_option") == "add_subfix":
            self.settings_dialog.output_add_subfix.setChecked(True)
        self.settings_dialog.output_subfix_input.setText(self.cwebp_settings.value("output_subfix"))
        self.settings_dialog.argument_input.setText(self.cwebp_settings.value("other_arguments"))

    def save_settings(self):
        for key, value in self.settings_dialog.arguments.items():
            self.cwebp_settings.setValue(key, value)

    def add_file(self):
        filename = QFileDialog.getOpenFileNames(
            self, "選擇檔案", filter="圖片 (*.png *.jpg *.jpeg *.tiff *.webp)"
        )
        for file in filename[0]:
            if file == "":
                continue
            item = QListWidgetItem()
            widget = FileItem(self, item, file)
            item.setIcon(self.style().standardIcon(QStyle.SP_FileIcon))
            item.setSizeHint(widget.sizeHint())
            self.queueing_files_list.addItem(item)
            self.queueing_files_list.setItemWidget(item, widget)

    def add_directory(self):
        dirname = QFileDialog.getExistingDirectory(self, "選擇資料夾")
        if dirname == "":
            return
        item = QListWidgetItem()
        widget = FileItem(self, item, dirname, True)
        item.setIcon(self.style().standardIcon(QStyle.SP_DirIcon))
        item.setSizeHint(widget.sizeHint())
        self.queueing_files_list.addItem(item)
        self.queueing_files_list.setItemWidget(item, widget)

    # called when the remove_button in widget is pressed
    def remove_list_item(self, item: QListWidgetItem):
        self.queueing_files_list.takeItem(self.queueing_files_list.indexFromItem(item).row())

    def open_setting(self):
        self.settings_dialog.open()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
