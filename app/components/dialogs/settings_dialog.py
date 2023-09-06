from PySide6.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QRadioButton,
    QSizePolicy,
    QSpacerItem,
    QVBoxLayout,
)


class SettingsDialog(QDialog):
    def __init__(self):
        super().__init__()

        self.quality_label = QLabel("品質（0 ~ 100）：")
        self.compress_option_label = QLabel("壓縮方式：")
        self.output_label = QLabel("輸出檔名：")
        self.argument_label = QLabel("其他參數：")

        self.quality_spin = QDoubleSpinBox(self)
        self.lossy_radio = QRadioButton("有損", self)
        self.lossless_radio = QRadioButton("無損", self)
        self.output_keep_name = QRadioButton("保持原檔名", self)
        self.output_add_subfix = QRadioButton("添加後綴：", self)
        self.output_subfix_input = QLineEdit(self)
        self.argument_input = QLineEdit(self)

        self.dialog_buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, self)

        self.quality_spacer = QSpacerItem(40, 20, hData=QSizePolicy.Expanding)
        self.compress_option_spacer = QSpacerItem(40, 20, hData=QSizePolicy.Expanding)

        self.main_layout = QVBoxLayout()
        self.quality_layout = QHBoxLayout()
        self.compress_option_layout = QHBoxLayout()
        self.output_layout = QHBoxLayout()
        self.argument_layout = QHBoxLayout()

        self.init_widgets()
        self.init_layouts()
        self.init_window()
        self.exec()

    def init_widgets(self):
        self.quality_spin.setMinimum(0)
        self.quality_spin.setMaximum(100)
        self.quality_spin.setValue(80)
        self.lossless_radio.setChecked(True)
        self.dialog_buttons.button(QDialogButtonBox.Ok).setText("確定")
        self.dialog_buttons.button(QDialogButtonBox.Cancel).setText("取消")

    def init_layouts(self):
        self.quality_layout.addWidget(self.quality_label)
        self.quality_layout.addWidget(self.quality_spin)
        self.quality_layout.addSpacerItem(self.quality_spacer)

        self.compress_option_layout.addWidget(self.compress_option_label)
        self.compress_option_layout.addWidget(self.lossy_radio)
        self.compress_option_layout.addWidget(self.lossless_radio)
        self.compress_option_layout.addSpacerItem(self.compress_option_spacer)

        self.output_layout.addWidget(self.output_label)
        self.output_layout.addWidget(self.output_keep_name)
        self.output_layout.addWidget(self.output_add_subfix)
        self.output_layout.addWidget(self.output_subfix_input)

        self.argument_layout.addWidget(self.argument_label)
        self.argument_layout.addWidget(self.argument_input)

        self.main_layout.addLayout(self.quality_layout)
        self.main_layout.addLayout(self.compress_option_layout)
        self.main_layout.addLayout(self.output_layout)
        self.main_layout.addLayout(self.argument_layout)
        self.main_layout.addWidget(self.dialog_buttons)

        self.setLayout(self.main_layout)

    def init_window(self):
        self.setFixedHeight(self.sizeHint().height())
        self.setWindowTitle("參數設定")
