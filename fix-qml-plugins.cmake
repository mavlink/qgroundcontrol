function(create_qml_plugin_targets)
    set(QML_PLUGINS
        qtquick2plugin
        qmlplugin
        modelsplugin
        workscriptplugin
        qtquickcontrols2plugin
        qtquickcontrols2fusionstyleplugin
        qtquickcontrols2materialstyleplugin
        qtquickcontrols2imaginestyleplugin
        qtquickcontrols2universalstyleplugin
        qtquickcontrols2fluentwinui3styleplugin
        qtquickcontrols2basicstyleplugin
        qtquicktemplates2plugin
        qtquickcontrols2implplugin
        qtquickcontrols2fusionstyleimplplugin
        quickwindowplugin
        qtquickcontrols2materialstyleimplplugin
        qtquickcontrols2imaginestyleimplplugin
        qtquickcontrols2universalstyleimplplugin
        qtquickcontrols2fluentwinui3styleimplplugin
        effectsplugin
        qquicklayoutsplugin
        qmlshapesplugin
        qtquickcontrols2basicstyleimplplugin
        labsmodelsplugin
        qtquickdialogsplugin
        labsplatformplugin
        qtchartsqml2plugin
        declarative_locationplugin
        positioningquickplugin
        labsanimationplugin
        qtgraphicaleffectsplugin
        qtqmlcoreplugin
        qtquickdialogs2quickimplplugin
        qtgraphicaleffectsprivateplugin
        quickmultimediaplugin
        qmlfolderlistmodelplugin
        qquick3dplugin
        workerscriptplugin
    )

    foreach(plugin ${QML_PLUGINS})
        if(NOT TARGET Qt6::${plugin})
            add_library(Qt6::${plugin} INTERFACE IMPORTED)
            set_target_properties(Qt6::${plugin} PROPERTIES
                INTERFACE_QT_QML_PLUGIN_TYPE "${plugin}"
            )
            message(STATUS "Created interface target: Qt6::${plugin}")
        endif()
    endforeach()
endfunction()

function(create_special_qml_targets)
    if(NOT TARGET Qt6::quickwindow)
        add_library(Qt6::quickwindow INTERFACE IMPORTED)
        set_target_properties(Qt6::quickwindow PROPERTIES
            INTERFACE_QT_QML_PLUGIN_TYPE "quickwindow"
        )
    endif()

    if(NOT TARGET Qt6::LabsPlatformplugin)
        add_library(Qt6::LabsPlatformplugin INTERFACE IMPORTED)
        set_target_properties(Qt6::LabsPlatformplugin PROPERTIES
            INTERFACE_QT_QML_PLUGIN_TYPE "labsplatform"
        )
    endif()

    if(NOT TARGET Qt6::qtchartsqml2)
        add_library(Qt6::qtchartsqml2 INTERFACE IMPORTED)
        set_target_properties(Qt6::qtchartsqml2 PROPERTIES
            INTERFACE_QT_QML_PLUGIN_TYPE "qtchartsqml2"
        )
    endif()

    if(NOT TARGET Qt6::declarative_location)
        add_library(Qt6::declarative_location INTERFACE IMPORTED)
        set_target_properties(Qt6::declarative_location PROPERTIES
            INTERFACE_QT_QML_PLUGIN_TYPE "declarative_location"
        )
    endif()

    if(NOT TARGET Qt6::qtgraphicaleffectsprivate)
        add_library(Qt6::qtgraphicaleffectsprivate INTERFACE IMPORTED)
        set_target_properties(Qt6::qtgraphicaleffectsprivate PROPERTIES
            INTERFACE_QT_QML_PLUGIN_TYPE "qtgraphicaleffectsprivate"
        )
    endif()

    if(NOT TARGET Qt6::quickmultimedia)
        add_library(Qt6::quickmultimedia INTERFACE IMPORTED)
        set_target_properties(Qt6::quickmultimedia PROPERTIES
            INTERFACE_QT_QML_PLUGIN_TYPE "quickmultimedia"
        )
    endif()
endfunction()

function(link_all_qml_plugins target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "Target ${target_name} not found")
        return()
    endif()

    set(ESSENTIAL_PLUGINS
        Qt6::qtqmlcoreplugin
        Qt6::qtquick2plugin
        Qt6::qmlplugin
        Qt6::qtquickcontrols2plugin
        Qt6::qtquicktemplates2plugin
        Qt6::qtgraphicaleffectsplugin
        Qt6::qquicklayoutsplugin
    )

    target_link_libraries(${target_name} 
        PRIVATE 
            ${ESSENTIAL_PLUGINS}
    )

    message(STATUS "Linked QML plugins to target: ${target_name}")
endfunction()

create_qml_plugin_targets()
create_special_qml_targets()
