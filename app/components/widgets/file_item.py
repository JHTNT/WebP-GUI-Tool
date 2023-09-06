import os

from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QLayout,
    QProgressBar,
    QSizePolicy,
    QSpacerItem,
    QWidget,
)


class CustomQWidget(QWidget):
    def __init__(self, path: str):
        super().__init__()

        self.file_path = QLabel(path, self)
        self.spacer = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)
        self.batch_progress = QProgressBar(self)
        self.batch_progress.setValue(0)
        self.batch_progress.setMaximum(self.count_file(path))
        self.batch_progress.setSizePolicy(QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed))
        self.status = QLabel("成功", self)

        self.horizontal_layout = QHBoxLayout()
        self.horizontal_layout.setSizeConstraint(QLayout.SetDefaultConstraint)
        self.horizontal_layout.addWidget(self.file_path)
        self.horizontal_layout.addItem(self.spacer)
        self.horizontal_layout.addWidget(self.batch_progress)
        self.horizontal_layout.addWidget(self.status)
        self.setLayout(self.horizontal_layout)

    def count_file(self, path: str):
        if os.path.isfile(path):
            return 1
        else:
            count = 0
            for item in os.listdir(path):
                if os.path.isfile(path + item):
                    count += 1
            return count
