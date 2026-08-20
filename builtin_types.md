## Sheet: Built-in Types

| type | UI platform | widget_type | Description | supported options |
| --- | --- | --- | --- | --- |
| bool | Qt | Toggle | A single checkable QpushButton. | label |
| bool | Qt | Checkbox | A standard QcheckBox. | label |
| bool | Qt | BinaryButtons | Two mutually exclusive push buttons representing true and false. | label, label_true, label_false |
| float | Qt | Input |  |  |
| float | Qt | Slider |  |  |
| float | Qt | Dial |  |  |
| float | Qt | ScrollBar |  |  |
| float | Qt | SliderFloat |  |  |
| std::string | Qt | SingleLineText (default) | Single-line text field with an Apply button. | label, placeholder, max_length |
| std::string | Qt | MultilineText | Multi-line text editor with an Apply button. | label, placeholder, min_lines, max_lines |
| std::string | Qt | CodeEditor | Fixed-width multi-line editor for code. | label, placeholder, min_lines, max_lines, tab_width |
| std::string | Qt | ComboBox (default when allowed_values is defined) | Drop-down list of allowed values. | label, allowed_values |
| std::string | Qt | ButtonGrid | Grid of selectable buttons. | label, allowed_values, columns, exclusive |
