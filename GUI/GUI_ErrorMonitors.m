function GUI_ErrorMonitors(varargin)
% GUI_ErrorMonitors defines the layout of the error monitors page

% Check if window already exists
if ~isempty(findobj('Tag','ErrMon'))
    figure(findobj('Tag','ErrMon'));
    return
end

% Initialization tasks
handles = guidata(gcbf);

% find longest motion
tEnd = 0.0;
for mo=1:length(handles.GM.dt)
    tEndi = handles.GM.scalet{mo}(end);
    if tEndi > tEnd
        tEnd = tEndi;
    end
end

%%%%%%%%%%%%%%%
%Create figure%
%%%%%%%%%%%%%%%

%Main Figure
SS = get(0,'screensize');
f_ErrorMon = figure('Visible','on','Name','Error Monitors',...
    'NumberTitle','off',...
    'MenuBar','none',...
    'Tag','ErrMon',...
    'Color',[0.3 0.5 0.7],...
    'KeyPressFcn',@shortcutKeys,...
    'Position',[SS(3)*0.12,SS(4)*0.05,SS(3)*0.88,SS(4)*0.87]);

%Toolbar
% set(f_ErrorMon,'Toolbar','figure');
% Toolbar_handles = findall(gcf);
% delete(Toolbar_handles([3:14 17:18]));
% set(Toolbar_handles(15), 'TooltipString', 'Print Window');
% set(Toolbar_handles(16), 'TooltipString', 'Save Window');
File(1) = uimenu('Position',1,'Label','File');
File(2) = uimenu(File(1),'Position',1,'Label','Save        Ctrl+S','Callback','filemenufcn(gcbf,''FileSaveAs'')');
File(3) = uimenu(File(1),'Position',2,'Label','Print        Ctrl+P','Callback','printdlg(gcbf)');
Separator(1) = uimenu('Position',2,'Label','|');
StdMenu(1) = uimenu('Position',3,'Label','MATLAB Menu');
StdMenu(2) = uimenu(StdMenu(1),'Position',1,'Label','Turn on', ...
   'Callback','set(gcf,''MenuBar'',''figure'');');
StdMenu(3) = uimenu(StdMenu(1),'Position',2,'Label','Turn off', ...
   'Callback','set(gcf,''MenuBar'',''none'');');


if strcmp(handles.Model.Type, '1 DOF')
    %1 DOF Case
    
    %Title
    uicontrol(f_ErrorMon,'Style','text',...
        'String','Error Monitors',...
        'FontSize',20,...
        'ForegroundColor',[1 1 1],...
        'BackgroundColor',[0.3 0.5 0.7],...
        'Units','normalized',...
        'Position',[0.3 0.87 0.4 0.1],...
        'FontName',handles.Store.font);
    
    %DOF 1 Figures
    a_ErrorMonitors_eX = axes('Parent',f_ErrorMon,...
        'Tag','EM1e',...
        'NextPlot','replacechildren',...
        'Position',[0.06 0.74 0.92 0.13],'Box','on',...
        'XLimMode','manual','Xlim',[0.0 tEnd]);
    grid('on');
    xlabel(a_ErrorMonitors_eX,'Time [sec]');
    ylabel(a_ErrorMonitors_eX,'Disp Error [L]');
    a_ErrorMonitors_efX = axes('Parent',f_ErrorMon,...
        'Tag','EM1ffte',...
        'NextPlot','replacechildren',...
        'Position',[0.06 0.54 0.92 0.13],'Box','on');
    grid('on');
    xlabel(a_ErrorMonitors_efX,'Frequency [Hz]');
    ylabel(a_ErrorMonitors_efX,'Fourier Amp [L/sec]');
    a_ErrorMonitors_ddX = axes('Parent',f_ErrorMon,...
        'Tag','EM1MeasCmd',...
        'NextPlot','replacechildren',...
        'Position',[0.06 0.07 0.42 0.41],'Box','on');
    %axis(a_ErrorMonitors_ddX,'equal');
    grid('on');
    xlabel(a_ErrorMonitors_ddX,'Command Disp [L]');
    ylabel(a_ErrorMonitors_ddX,'Measured Disp [L]');
    a_ErrorMonitors_etX = axes('Parent',f_ErrorMon,...
        'Tag','EM1track',...
        'NextPlot','replacechildren',...
        'Position',[0.56 0.07 0.42 0.41],'Box','on',...
        'XLimMode','manual','Xlim',[0.0 tEnd]);
    grid('on');
    xlabel(a_ErrorMonitors_etX,'Time [sec]');
    ylabel(a_ErrorMonitors_etX,'Tracking Indicator [L^2]');
    
else
    %2 DOF Case
%     set(f_ErrorMon,'Position',[SS(3)*0.12,0,SS(3)*0.88,SS(4)]);
    
    %Title
    uicontrol(f_ErrorMon,'Style','text',...
        'String','Error Monitors',...
        'FontSize',20,...
        'ForegroundColor',[1 1 1],...
        'BackgroundColor',[0.3 0.5 0.7],...
        'Units','normalized',...
        'Position',[0.3 0.92 0.4 0.05]);
    %DOF 1 Figures
    uicontrol(f_ErrorMon,'Style','text',...
        'Units','normalized',...
        'FontSize',15,...
        'ForegroundColor',[1 1 1],...
        'BackgroundColor',[0.3 0.5 0.7],...
        'String','DOF 1',...
        'Position',[0.26 0.85 0.1 0.05]);
    a_ErrorMonitors_eX = axes('Parent',f_ErrorMon,...
        'Tag','EM1e',...
        'NextPlot','replacechildren',...
        'Position',[0.06 0.72 0.43 0.13],'Box','on',...
        'XLimMode','manual','Xlim',[0.0 tEnd]);
    grid('on');
    xlabel(a_ErrorMonitors_eX,'Time [sec]');
    ylabel(a_ErrorMonitors_eX,'Disp Error [L]');    
    a_ErrorMonitors_efX = axes('Parent',f_ErrorMon,...
        'Tag','EM1ffte',...
        'NextPlot','replacechildren',...
        'Position',[0.06 0.52 0.43 0.13],'Box','on');
    grid('on');
    xlabel(a_ErrorMonitors_efX,'Frequency [Hz]');
    ylabel(a_ErrorMonitors_efX,'Fourier Amp [L/sec]');
    a_ErrorMonitors_ddX = axes('Parent',f_ErrorMon,...
        'Tag','EM1MeasCmd',...
        'NextPlot','replacechildren',...
        'Position',[0.06 0.08 0.19 0.38],'Box','on');
    %axis(a_ErrorMonitors_ddX,'equal');
    grid('on');
    xlabel(a_ErrorMonitors_ddX,'Command Disp [L]');
    ylabel(a_ErrorMonitors_ddX,'Measured Disp [L]');
    a_ErrorMonitors_etX = axes('Parent',f_ErrorMon,...
        'Tag','EM1track',...
        'NextPlot','replacechildren',...
        'Position',[0.3 0.08 0.19 0.38],'Box','on',...
        'XLimMode','manual','Xlim',[0.0 tEnd]);
    grid('on');
    xlabel(a_ErrorMonitors_etX,'Time [sec]');
    ylabel(a_ErrorMonitors_etX,'Tracking Indicator [L^2]');
    
    %DOF 2 Figures
    uicontrol(f_ErrorMon,'Style','text',...
        'Units','normalized',...
        'FontSize',15,...
        'ForegroundColor',[1 1 1],...
        'BackgroundColor',[0.3 0.5 0.7],...
        'String','DOF 2',...
        'Position',[0.72 0.85 0.1 0.05]);
    a_ErrorMonitors_eY = axes('Parent',f_ErrorMon,...
        'Tag','EM2e',...
        'NextPlot','replacechildren',...
        'Position',[0.55 0.72 0.43 0.13],'Box','on',...
        'XLimMode','manual','Xlim',[0.0 tEnd]);
    grid('on');
    xlabel(a_ErrorMonitors_eY,'Time [sec]');
    ylabel(a_ErrorMonitors_eY,'Disp Error [L]');
    a_ErrorMonitors_efY = axes('Parent',f_ErrorMon,...
        'Tag','EM2ffte',...
        'NextPlot','replacechildren',...
        'Position',[0.55 0.52 0.43 0.13],'Box','on');
    grid('on');
    xlabel(a_ErrorMonitors_efY,'Frequency [Hz]');
    ylabel(a_ErrorMonitors_efY,'Fourier Amp [L/sec]');
    a_ErrorMonitors_ddY = axes('Parent',f_ErrorMon,...
        'Tag','EM2MeasCmd',...
        'NextPlot','replacechildren',...
        'Position',[0.55 0.08 0.19 0.38],'Box','on');
    %axis(a_ErrorMonitors_ddY,'equal');
    grid('on');
    xlabel(a_ErrorMonitors_ddY,'Command Disp [L]');
    ylabel(a_ErrorMonitors_ddY,'Measured Disp [L]');
    a_ErrorMonitors_etY = axes('Parent',f_ErrorMon,...
        'Tag','EM2track',...
        'NextPlot','replacechildren',...
        'Position',[0.79 0.08 0.19 0.38],'Box','on',...
        'XLimMode','manual','Xlim',[0.0 tEnd]);
    grid('on');
    xlabel(a_ErrorMonitors_etY,'Time [sec]');
    ylabel(a_ErrorMonitors_etY,'Tracking Indicator [L^2]');
end

%Callback for shortcut keys
    function shortcutKeys(source, eventdata)
        control = 0;
        for x=1:length(eventdata.Modifier)
            switch(eventdata.Modifier{x})
                case 'control'
                    control = 1;
            end
        end
        if (control == 1 && strcmp(eventdata.Key,'s'))
            filemenufcn(gcbf,'FileSaveAs');
        elseif (control == 1 && strcmp(eventdata.Key,'p'))
            printdlg(gcbf);
        end
    end
end
