// ChatTabSettingsWidget.cpp
#include "ChatTabSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "ChatTab.h"

void UChatTabSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind button handlers
    if (ApplyButton)
    {
        ApplyButton->OnClicked.AddDynamic(this, &UChatTabSettingsWidget::OnApplyButtonClicked);
    }
    
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UChatTabSettingsWidget::OnCancelButtonClicked);
    }
    
    if (DeleteButton)
    {
        DeleteButton->OnClicked.AddDynamic(this, &UChatTabSettingsWidget::OnDeleteButtonClicked);
    }
    
    // Create channel options UI
    CreateChannelOptions();
}

void UChatTabSettingsWidget::SetTab(UChatTab* InTab)
{
    CurrentTab = InTab;
    
    if (!CurrentTab)
    {
        UE_LOG(LogTemp, Error, TEXT("ChatTabSettingsWidget::SetTab - No tab provided"));
        return;
    }
    
    // Set current tab name
    if (TabNameInput)
    {
        TabNameInput->SetText(FText::FromString(CurrentTab->TabName));
    }
    
    // Update channel checkboxes based on tab's settings
    UpdateCheckboxStates();
}

void UChatTabSettingsWidget::ApplySettings()
{
    if (!CurrentTab)
    {
        UE_LOG(LogTemp, Error, TEXT("ChatTabSettingsWidget::ApplySettings - No current tab"));
        return;
    }
    
    // Update tab name
    if (TabNameInput)
    {
        CurrentTab->TabName = TabNameInput->GetText().ToString();
    }
    
    // Update channel visibility settings - this is handled by checkbox callbacks
    
    // Broadcast the event
    OnSettingsApplied.Broadcast();
}

void UChatTabSettingsWidget::CancelSettings()
{
    // Broadcast the cancel event
    OnSettingsCancelled.Broadcast();
}

void UChatTabSettingsWidget::DeleteTab()
{
    // Broadcast the delete event
    OnTabDeleted.Broadcast();
}

void UChatTabSettingsWidget::OnApplyButtonClicked()
{
    ApplySettings();
}

void UChatTabSettingsWidget::OnCancelButtonClicked()
{
    CancelSettings();
}

void UChatTabSettingsWidget::OnDeleteButtonClicked()
{
    DeleteTab();
}

void UChatTabSettingsWidget::CreateChannelOptions()
{
    if (!ChannelOptionsBox)
    {
        UE_LOG(LogTemp, Error, TEXT("ChatTabSettingsWidget::CreateChannelOptions - No ChannelOptionsBox"));
        return;
    }
    
    // Clear existing options
    ChannelOptionsBox->ClearChildren();
    ChannelCheckboxes.Empty();
    
    // Create a checkbox for each channel type
    TArray<TPair<EChatChannel, FString>> ChannelOptions = {
        {EChatChannel::Say, TEXT("Say")},
        {EChatChannel::Shout, TEXT("Shout")},
        {EChatChannel::Tell, TEXT("Whisper")},
        {EChatChannel::Group, TEXT("Group")},
        {EChatChannel::Guild, TEXT("Guild")},
        {EChatChannel::Trade, TEXT("Trade")},
        {EChatChannel::OOC, TEXT("OOC")},
        {EChatChannel::General, TEXT("General")},
        {EChatChannel::Region, TEXT("Region")}
    };
    
    for (const TPair<EChatChannel, FString>& Option : ChannelOptions)
    {
        // Create horizontal box for this option
        UHorizontalBox* OptionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        
        // Create checkbox
        UCheckBox* Checkbox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
        Checkbox->SetIsChecked(true); // Default to showing all channels
        
        // Create label
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(FText::FromString(Option.Value));
        
        // We don't need to store the channel info with the checkbox
        // Instead, we'll use the ChannelCheckboxes map to look it up when needed
        
        // Add checkbox and label to the row
        OptionRow->AddChild(Checkbox);
        OptionRow->AddChild(Label);
        
        // Add row to options box
        ChannelOptionsBox->AddChild(OptionRow);
        
        // Store checkbox for later use
        ChannelCheckboxes.Add(Option.Key, Checkbox);
        
        // Bind to checkbox change event
        Checkbox->OnCheckStateChanged.AddDynamic(this, &UChatTabSettingsWidget::OnChannelCheckboxChanged);
    }
}

void UChatTabSettingsWidget::UpdateCheckboxStates()
{
    if (!CurrentTab)
        return;
        
    for (auto& Pair : ChannelCheckboxes)
    {
        EChatChannel Channel = Pair.Key;
        UCheckBox* Checkbox = Pair.Value;
        
        if (Checkbox)
        {
            Checkbox->SetIsChecked(CurrentTab->IsChannelVisible(Channel));
        }
    }
}

void UChatTabSettingsWidget::OnChannelCheckboxChanged(bool bIsChecked)
{
    if (!CurrentTab)
        return;
    
    // Since we can't directly identify which checkbox called this function,
    // we'll need to update all channels' visibility based on all checkboxes
    for (auto& Pair : ChannelCheckboxes)
    {
        EChatChannel Channel = Pair.Key;
        UCheckBox* Checkbox = Pair.Value;
        
        if (Checkbox)
        {
            CurrentTab->SetChannelVisibility(Channel, Checkbox->IsChecked());
        }
    }
}