import os

from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QLayout,
    QProgressBar,
    QPushButton,
    QSizePolicy,
    QSpacerItem,
    QStyle,
    QWidget,
)


class FileItem(QWidget):
    def __init__(self, parent, item, path: str, is_directory: bool = False):
        super().__init__()

        self._parent = parent
        self.item = item

        self.file_path = QLabel(path, self)

        self.spacer = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.batch_progress = QProgressBar(self)
        self.batch_progress.setFormat("%v/%m")
        self.batch_progress.setValue(0)
        if is_directory:
            self.file_cnt = self.count_file(path)
        else:
            self.file_cnt = 1
        self.batch_progress.setMaximum(self.file_cnt)
        self.batch_progress.setSizePolicy(QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed))

        self.status = QLabel("等待壓縮", self)

        self.remove_button = QPushButton(
            icon=self.style().standardIcon(QStyle.SP_TitleBarCloseButton)
        )
        self.remove_button.setToolTip("從佇列移除")

        self.horizontal_layout = QHBoxLayout()
        self.horizontal_layout.setSizeConstraint(QLayout.SetDefaultConstraint)
        self.horizontal_layout.addWidget(self.file_path)
        self.horizontal_layout.addItem(self.spacer)
        self.horizontal_layout.addWidget(self.batch_progress)
        self.horizontal_layout.addWidget(self.status)
        self.horizontal_layout.addWidget(self.remove_button)
        self.setLayout(self.horizontal_layout)

        self.remove_button.clicked.connect(self.remove_item)

    def count_file(self, path: str):
        count = 0
        for item in os.listdir(path):
            if os.path.isfile(path + "/" + item) and item.endswith(
                (".png", ".jpg", ".jpeg", ".tiff", ".webp")
            ):
                count += 1
        return count

    def remove_item(self):
        self._parent.remove_list_item(self.item)
        self.deleteLater()
