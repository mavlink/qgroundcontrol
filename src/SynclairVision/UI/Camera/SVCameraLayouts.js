function getCameraLayouts() {
    return [
        {
            id: 'single',
            name: 'Single',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 1.0, h: 1.0},
            ],
            separators: [],
        },
        {
            id: 'two_stacked_square',
            name: '2 Stacked Square',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 1.0, h: 0.5},
                {slot: 1, x: 0.0, y: 0.5, w: 1.0, h: 0.5},
            ],
            separators: [
                {orientation: 'horizontal', x: 0.0, y: 0.5, length: 1.0},
            ],
        },
        {
            id: 'two_stacked_panorama',
            name: '2 Stacked Panorama',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 1.0, h: 0.5},
                {slot: 1, x: 0.0, y: 0.5, w: 1.0, h: 0.5},
            ],
            separators: [
                {orientation: 'horizontal', x: 0.0, y: 0.5, length: 1.0},
            ],
        },
        {
            id: 'two_square_one_panorama',
            name: '2 Square, 1 Panorama',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 1.0, h: 0.5},
                {slot: 1, x: 0.0, y: 0.5, w: 1.0, h: 0.5},
            ],
            separators: [
                {orientation: 'horizontal', x: 0.0, y: 0.5, length: 1.0},
            ],
        },
        {
            id: 'four_square',
            name: '4 Square',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 0.5, h: 0.5},
                {slot: 1, x: 0.5, y: 0.0, w: 0.5, h: 0.5},
                {slot: 2, x: 0.0, y: 0.5, w: 0.5, h: 0.5},
                {slot: 3, x: 0.5, y: 0.5, w: 0.5, h: 0.5},
            ],
            separators: [
                {orientation: 'vertical', x: 0.5, y: 0.0, length: 1.0},
                {orientation: 'horizontal', x: 0.0, y: 0.5, length: 1.0},
            ],
        },
        {
            id: 'three_square_one_panorama',
            name: '3 Square, 1 Panorama',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 0.5, h: 0.5},
                {slot: 1, x: 0.5, y: 0.0, w: 0.5, h: 0.5},
                {slot: 2, x: 0.0, y: 0.5, w: 0.5, h: 0.5},
                {slot: 3, x: 0.5, y: 0.5, w: 0.5, h: 0.5},
            ],
            separators: [
                {orientation: 'vertical', x: 0.5, y: 0.0, length: 1.0},
                {orientation: 'horizontal', x: 0.0, y: 0.5, length: 1.0},
            ],
        },
        {
            id: 'entire_picture',
            name: 'Entire Picture',
            panes: [
                {slot: 0, x: 0.0, y: 0.0, w: 1.0, h: 1.0},
            ],
            separators: [],
        },
    ];
}
