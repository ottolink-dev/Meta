## Sheet: Built-in Types

| type | UI platform | widget_type | Description | supported options |
| --- | --- | --- | --- | --- |
| bool | Qt | Toggle (default) | A single checkable QPushButton. | label |
| bool | Qt | Checkbox | A standard QCheckBox. | label |
| bool | Qt | BinaryButtons | Two mutually exclusive push buttons representing true and false. | label, label_true, label_false |
| int | Qt | Input (default) | Spin box for integer input. | label, min, max, step |
| int | Qt | EnumComboBox | Drop-down list for enum values. | label |
| int | Qt | Slider | Horizontal slider widget. | label, min, max |
| int | Qt | ScrollBar | Horizontal scroll bar widget. | label, min, max |
| int | Qt | Dial | Circular dial widget. | label, min, max |
| int | Qt | SliderInt | Custom slider widget with spinbox and +/- buttons. | label, min, max, plus_minus, format |
| float | Qt | Input (default) | Spin box for floating-point input. | label, min, max, step, format |
| float | Qt | Slider | Horizontal slider widget. | label, min, max |
| float | Qt | ScrollBar | Horizontal scroll bar widget. | label, min, max |
| float | Qt | Dial | Circular dial widget. | label, min, max |
| float | Qt | SliderFloat | Custom slider widget with spinbox, +/- buttons, and log scale support. | label, min, max, plus_minus, format, log_scale |
| std::string | Qt | SingleLineText (default) | Single-line text field with an Apply button. | label, placeholder, max_length |
| std::string | Qt | MultilineText | Multi-line text editor with an Apply button. | label, placeholder, min_lines, max_lines |
| std::string | Qt | CodeEditor | Fixed-width multi-line editor for code with an Apply button. | label, placeholder, min_lines, max_lines, tab_width |
| std::string | Qt | ReadOnlyText | Read-only text label display. | label |
| std::string | Qt | ComboBox (default when allowed_values is defined) | Drop-down list of allowed values. | label, allowed_values |
| std::string | Qt | ButtonGrid | Grid of selectable buttons. | label, allowed_values, columns, exclusive |
| std::filesystem::path | Qt | OpenFile (default) | File open dialog picker with path line display and clear button. | label, file_filter, start_dir |
| std::filesystem::path | Qt | SaveFile | File save dialog picker with path line display and clear button. | label, file_filter, start_dir |
| std::filesystem::path | Qt | Directory | Directory selection dialog picker with path line display and clear button. | label, start_dir |
| glm::ivec2 | Qt | Input (default) | Spin boxes for X and Y integer components. | label, min, max, step, power_of_two, aspect_ratio |
| glm::vec2 | Qt | Input (default) | Spin boxes for X and Y floating-point components. | label, min, max, step, format |
| glm::vec2 | Qt | XYCanvas | Interactive 2D coordinate grid canvas. | label, min, max, min_x, max_x, min_y, max_y, show_grid |
| glm::vec2 | Qt | RangeBar | Dual-handle range selection bar with histogram support. | label, min, max, format, has_active_toggle, active, data_provider |
| glm::vec2 | Qt | VectorEditor | Interactive 2D vector canvas with magnitude and angle controls. | label, max, locked_xy, format |
| glm::vec2 | Qt | LinkedSliders | Dual linked float sliders with X=Y lock button. | label, min, max, format, locked_xy, label_x, label_y |
| glm::vec3 | Qt | Input (default) | Spin boxes for X, Y, and Z floating-point components. | label, min, max, step, format |
| glm::vec3 | Qt | ColorPicker | Color swatch button opening a color dialog with RGB hex display. | label |
| glm::vec4 | Qt | Input (default) | Spin boxes for X, Y, Z, and W floating-point components. | label, min, max, step, format |
| glm::vec4 | Qt | ColorPicker | Color swatch button opening a color dialog with RGBA alpha support and hex display. | label |
| std::vector<float> | Qt | CurveEditor (default) | Interactive 2D curve canvas editor. | label, curve_size, min_x, max_x, min_y, max_y |
| std::vector<glm::vec3> | Qt | PointsEditor (default) | Interactive 2D/3D points canvas editor with toolbar actions and background image provider. | label, min_x, max_x, min_y, max_y, z_step, closed, data_provider |
| std::vector<glm::vec3> | Qt | PathEditor | Interactive connected path editor canvas with toolbar actions and background image provider. | label, min_x, max_x, min_y, max_y, z_step, closed, data_provider |
| meta::ColorGradient | Qt | GradientEditor (default) | Interactive color gradient picker with stops and presets. | label, presets |
| meta::Array | Qt | ArrayEditor (default) | 2D array drawing canvas with background image provider support. | label, width, height, data_provider |
